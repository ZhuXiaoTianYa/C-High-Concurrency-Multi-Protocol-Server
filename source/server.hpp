#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <cassert>
#include <string>
#include <cstring>
#include <iostream>
#include <ctime>
#include <cstdarg>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

#define DBG 0
#define INF 1
#define ERR 2
#define FAT 3
#define LOG_LEVEL DBG

#define LOG(level, format, ...)                                                               \
    do                                                                                        \
    {                                                                                         \
        if (level < LOG_LEVEL)                                                                \
            break;                                                                            \
        time_t t = time(NULL);                                                                \
        struct tm *ltm = localtime(&t);                                                       \
        char timer[32] = {0};                                                                 \
        strftime(timer, 31, "%H:%M:%S", ltm);                                                 \
        fprintf(stdout, "[%s %s:%d] " format "\n", timer, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)
#define FAT_LOG(format, ...) LOG(FAT, format, ##__VA_ARGS__)

#define BUFFER_DEFAULT_SIZE 1024
class Buffer
{
private:
    uint64_t _read_idx;
    uint64_t _write_idx;
    std::vector<char> _buffer;

private:
    char *Begin() { return &*_buffer.begin(); }

public:
    Buffer() : _read_idx(0), _write_idx(0), _buffer(BUFFER_DEFAULT_SIZE) {}
    char *WritePosition() { return Begin() + _write_idx; }
    char *ReadPosition() { return Begin() + _read_idx; }
    uint64_t TailIdleSize() const { return _buffer.size() - _write_idx; }
    uint64_t HeadIdleSize() const { return _read_idx; }
    uint64_t ReadAbleSize() const { return _write_idx - _read_idx; }
    void MoveReadOffset(const uint64_t &len)
    {
        assert(len <= ReadAbleSize());
        _read_idx += len;
    }
    void MoveWriteOffset(const uint64_t &len)
    {
        assert(len <= TailIdleSize());
        _write_idx += len;
    }
    void EnsureWriteSpace(const uint64_t &len)
    {
        if (TailIdleSize() >= len)
            return;
        if (TailIdleSize() + HeadIdleSize() >= len)
        {
            uint64_t rsz = ReadAbleSize();
            std::copy(ReadPosition(), ReadPosition() + rsz, Begin());
            _write_idx = rsz;
            _read_idx = 0;
            return;
        }
        else
        {
            _buffer.resize(_write_idx + len);
            return;
        }
    }
    void Write(const void *data, const uint64_t &len)
    {
        EnsureWriteSpace(len);
        const char *d = (const char *)data;
        std::copy(d, d + len, WritePosition());
    }
    void WriteString(const std::string &data)
    {
        Write(data.c_str(), data.size());
    }
    void WriteBuffer(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
    }
    void WriteAndMove(const void *data, const uint64_t &len)
    {
        Write(data, len);
        MoveWriteOffset(len);
    }
    void WriteStringAndMove(const std::string &data)
    {
        Write(&data[0], data.size());
        MoveWriteOffset(data.size());
    }
    void WriteBufferAndMove(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
        MoveWriteOffset(data.ReadAbleSize());
    }
    void Read(void *buf, const uint64_t &len)
    {
        assert(len <= ReadAbleSize());
        std::copy(ReadPosition(), ReadPosition() + len, (char *)buf);
    }
    std::string ReadAsString(const uint64_t &len)
    {
        assert(len <= ReadAbleSize());
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }
    void ReadAndMove(void *buf, const uint64_t &len)
    {
        Read(buf, len);
        MoveReadOffset(len);
    }
    std::string ReadAsStringAndMove(const uint64_t &len)
    {
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        MoveReadOffset(len);
        return str;
    }
    char *FindCRLF()
    {
        void *ret = memchr(ReadPosition(), '\n', ReadAbleSize());
        return (char *)ret;
    }

    std::string GetLine()
    {
        char *pos = FindCRLF();
        if (pos == NULL)
        {
            return "";
        }
        return ReadAsString(pos - ReadPosition() + 1);
    }

    std::string GetLineAndMove()
    {
        std::string str = GetLine();
        MoveReadOffset(str.size());
        return str;
    }
    void Clear()
    {
        _write_idx = 0;
        _read_idx = 0;
    }
};

#define MAX_LISTEN 1024

class Socket
{
private:
    int _sockfd;

public:
    Socket() : _sockfd(-1) {}
    Socket(const int &sockfd) : _sockfd(sockfd) {}
    ~Socket() { Close(); }
    int Fd() { return _sockfd; }
    bool Create()
    {
        int newfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newfd < 0)
        {
            ERR_LOG("CREATE SOCKET FAILED!");
            return false;
        }
        _sockfd = newfd;
        return true;
    }
    bool Bind(const std::string &ip, const uint16_t &port)
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        socklen_t len = sizeof(addr);
        int ret = bind(_sockfd, (sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("BIND SOCKET FAILED!");
            return false;
        }
        return true;
    }

    bool Listen(const int &backlog = MAX_LISTEN)
    { // 相当于内核标记这个套接字为监听套接字
        // 内核的工作模式就变成了：
        // 内核自动接管三次握手
        // 自动维护一个半连接SYN队列和一个全连接Accept队列
        // 这个时候内核就开始收SYN
        // 回SYN+ACK
        // 等最后的ACK

        int ret = listen(_sockfd, backlog);
        if (ret == -1)
        {
            FAT_LOG("LISTEN SOCKET FAILED!");
            return false;
        }
        return true;
    }

    bool Connect(const std::string &ip, const uint16_t &port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        socklen_t len = sizeof(addr);
        int ret = connect(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("CONNECT FAILED!");
            return false;
        }
        return true;
    }

    int Accept()
    {
        // 相当于从全连接队列中拿出一个已经完成握手的客户端，然后创建一个全新的套接字给他
        int ret = accept(_sockfd, NULL, NULL);
        if (ret == -1)
        {
            ERR_LOG("ACCEPT SOCKET FAILED!");
            return ret;
        }
        return ret;
    }
    ssize_t Recv(void *buf, const size_t &len, const int &flag = 0)
    {
        ssize_t ret = recv(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            if (ret == EAGAIN || ret == EINTR)
            {
                return 0;
            }
            return -1;
        }
        return ret;
    }

    ssize_t NonBlockRecv(void *buf, const size_t &len)
    {
        return Recv(buf, len, MSG_DONTWAIT);
    }
    ssize_t Send(const void *buf, const size_t &len, const int &flag = 0)
    {
        ssize_t ret = send(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            if (ret == EAGAIN || ret == EINTR)
            {
                return 0;
            }
            return -1;
        }
        return ret;
    }

    ssize_t NonBlockSend(void *buf, const size_t &len)
    {
        return Send(buf, len, MSG_DONTWAIT);
    }
    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }
    bool CreateServer(const uint16_t &port, const std::string &ip = "0.0.0.0", bool block_flag = true)
    {
        if (Create() == false)
            return false;
        ReuseAddress();
        if (Bind(ip, port) == false)
            return false;
        if (Listen() == false)
            return false;
        if (block_flag == false)
            NonBlock();
        return true;
    }
    bool CreateClient(const uint16_t &port, const std::string &ip)
    {
        if (Create() == false)
            return false;
        if (Connect(ip, port) == false)
            return false;
        return true;
    }
    void ReuseAddress()
    {
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(int));
    }
    void NonBlock()
    {
        int flag = fcntl(_sockfd, F_GETFD, 0);
        fcntl(_sockfd, F_SETFD, flag | O_NONBLOCK);
    }
};

class Poller;
class EventLoop;
class Channel
{
    using EventCallback = std::function<void()>;

public:
    Channel(EventLoop *loop, const int &fd) : _loop(loop), _fd(fd), _events(0), _revents(0) {}
    int Fd() { return _fd; }
    uint32_t Events() { return _events; }
    void SetREvents(const uint32_t &events) { _revents = events; }
    void SetReadCallback(const EventCallback &cb) { _read_callback = cb; }
    void SetWriteCallback(const EventCallback &cb) { _write_callback = cb; }
    void SetErrorCallback(const EventCallback &cb) { _error_callback = cb; }
    void SetCloseCallback(const EventCallback &cb) { _close_callback = cb; }
    void SetEventCallback(const EventCallback &cb) { _event_callback = cb; }
    bool ReadAble() { return _events & EPOLLIN; }
    bool WriteAble() { return _events & EPOLLOUT; }
    void EnableRead()
    {
        _events |= EPOLLIN;
        Update();
    }
    void EnableWrite()
    {
        _events |= EPOLLOUT;
        Update();
    }
    void DisableRead()
    {
        _events &= ~EPOLLIN;
        Update();
    }
    void DisableWrite()
    {
        _events &= ~EPOLLOUT;
        Update();
    }
    void DisableAll()
    {
        _events = 0;
        Update();
    }
    void Update();
    void Remove();
    void HandleEvent()
    {
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI))
        {
            if (_read_callback)
                _read_callback();
            if (_event_callback)
                _event_callback();
        }
        if (_revents & EPOLLOUT)
        {
            if (_write_callback)
                _write_callback();
            if (_event_callback)
                _event_callback();
        }
        else if (_revents & EPOLLERR)
        {
            if (_event_callback)
                _event_callback();
            if (_error_callback)
                _error_callback();
        }
        else if (_revents & EPOLLHUP)
        {
            if (_event_callback)
                _event_callback();
            if (_close_callback)
                _close_callback();
        }
    }

private:
    int _fd;
    EventLoop *_loop;
    uint32_t _events;
    uint32_t _revents;
    EventCallback _read_callback;
    EventCallback _write_callback;
    EventCallback _error_callback;
    EventCallback _close_callback;
    EventCallback _event_callback;
};

#define MAX_EPOLLEVENTS 1024
class Poller
{
private:
    void Update(Channel *channel, const int &op)
    {
        int fd = channel->Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = channel->Events();
        // DBG_LOG("fd: %d, events: %d", fd, ev.events);
        int ret = epoll_ctl(_epfd, op, fd, &ev);
        if (ret < 0)
        {
            ERR_LOG("EPOLL CTL FAILED!");
            return;
        }
        return;
    }
    bool HasChannel(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it == _channels.end())
        {
            return false;
        }
        return true;
    }

public:
    Poller() : _epfd(-1)
    {
        _epfd = epoll_create(1);
        if (_epfd < 0)
        {
            FAT_LOG("EPOLL CREATE FAILED");
            _epfd = -1;
            abort();
        }
    }
    void UpdateEvent(Channel *channel)
    {
        if (HasChannel(channel) == false)
        {
            _channels.insert(std::make_pair(channel->Fd(), channel));
            Update(channel, EPOLL_CTL_ADD);
        }
        Update(channel, EPOLL_CTL_MOD);
    }
    void RemoveEvent(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it != _channels.end())
        {
            _channels.erase(it);
        }
        Update(channel, EPOLL_CTL_DEL);
    }
    void Poll(std::vector<Channel *> *active)
    {
        int nfds = epoll_wait(_epfd, _evs, MAX_EPOLLEVENTS, -1);
        if (nfds < 0)
        {
            if (nfds == EINTR)
                return;
            ERR_LOG("EPOLL WAIT FAILED!");
            abort();
        }
        for (int i = 0; i < nfds; i++)
        {
            auto it = _channels.find(_evs[i].data.fd);
            assert(it != _channels.end());
            it->second->SetREvents(_evs->events);
            active->push_back(it->second);
        }
    }

private:
    int _epfd;
    struct epoll_event _evs[MAX_EPOLLEVENTS];
    std::unordered_map<int, Channel *> _channels;
};

class EventLoop;
using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimerTask
{
public:
    TimerTask(const uint64_t &id, const uint32_t &timeout, const TaskFunc &cb) : _id(id), _timeout(timeout), _task_cb(cb), _canceled(false) {}
    ~TimerTask()
    {
        if (_canceled == false)
            _task_cb();
        _release();
    }
    void Cancel()
    {
        _canceled = true;
    }
    void SetRelease(const ReleaseFunc &cb)
    {
        _release = cb;
    }
    uint32_t DelayTime()
    {
        return _timeout;
    }

private:
    TaskFunc _task_cb;
    ReleaseFunc _release;
    bool _canceled;
    uint64_t _id;
    uint32_t _timeout;
};

class TimerWheel
{
    using PtrTask = std::shared_ptr<TimerTask>;
    using WeakTask = std::weak_ptr<TimerTask>;

private:
    static int CreateTimerfd()
    {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0)
        {
            FAT_LOG("TIMERFD CREATE FAILED!");
            abort();
        }
        struct itimerspec itime;
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0;
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0;
        int ret = timerfd_settime(timerfd, 0, &itime, NULL);
        if (ret < 0)
        {
            FAT_LOG("TIMERFD SETTIME FAILED!");
            abort();
        }
        return timerfd;
    }

    void RunTimerTask()
    {
        _tick = (_tick + 1) % _capcity;
        _wheel[_tick].clear();
    }
    void ReadTimerfd()
    {
        uint64_t val = 0;
        int ret = read(_timerfd, &val, 8);
        if (ret < 0)
        {
            // if(ret==EAGAIN||ret==EINTR)
            //     return;
            FAT_LOG("READ TIMERFD FAILED!");
            abort();
        }
        return;
    }
    void OnTime()
    {
        ReadTimerfd();
        RunTimerTask();
    }
    void TimerAddInLoop(const uint64_t &id, const uint32_t &delay, const TaskFunc &cb)
    {
        PtrTask pt(new TimerTask(id, delay, cb));
        pt->SetRelease(std::bind(&TimerWheel::RemoveTimer, this, id));
        _timers[id] = WeakTask(pt);
        int pos = (_tick + delay) % _capcity;
        _wheel[pos].push_back(pt);
    }
    void TimerRefreshInLoop(const uint64_t &id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return;
        PtrTask pt = it->second.lock();
        int pos = (_tick + pt->DelayTime()) % _capcity;
        _wheel[pos].push_back(pt);
    }
    void TimerCancelInLoop(const uint64_t &id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return;
        }
        PtrTask pt = it->second.lock();
        pt->Cancel();
    }

public:
    TimerWheel(EventLoop *loop, const int &capcity = 60) : _tick(0), _capcity(capcity), _timerfd(CreateTimerfd()), _wheel(_capcity), _loop(loop), _timer_channel(new Channel(_loop, _timerfd))
    {
        _timer_channel->SetReadCallback(std::bind(&TimerWheel::OnTime, this));
        _timer_channel->EnableRead();
    }
    void TimerAdd(const uint64_t &id, const uint32_t &delay, const TaskFunc &cb);
    void TimerRefresh(const uint64_t &id);
    void TimerCancel(const uint64_t &id);
    // 存在线程安全问题，要保证只能在EventLoop中的线程中调用
    bool HasTimer(const uint64_t &id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return false;
        return true;
    }

private:
    void RemoveTimer(const uint64_t &id)
    {
        auto it = _timers.find(id);
        if (it != _timers.end())
            _timers.erase(it);
    }

private:
    int _tick;
    int _capcity;
    int _timerfd;
    EventLoop *_loop;
    std::unique_ptr<Channel> _timer_channel;
    std::vector<std::vector<PtrTask>> _wheel;
    std::unordered_map<uint64_t, WeakTask> _timers;
};

class EventLoop
{
    using Functor = std::function<void()>;

private:
    static int CreateEventFd()
    {
        int efd = eventfd(0, 0);
        if (efd < 0)
        {
            FAT_LOG("EVENTFD FAILED!");
            abort();
        }
        return efd;
    }
    void ReadEventFd()
    {
        uint64_t val;
        ssize_t ret = read(_event_fd, &val, 8);
        if (ret <= 0)
        {
            if (ret == EINTR || ret == EAGAIN)
            {
                return;
            }
            FAT_LOG("READ EVENTFA FAILED!");
            abort();
        }
        return;
    }
    void WakeupEventFd()
    {
        uint64_t val = 1;
        int ret = write(_event_fd, &val, 8);
        if (ret < 0)
        {
            if (ret == EINTR)
            {
                return;
            }
            FAT_LOG("EVENTFD WRITE FAILED!");
            abort();
        }
        return;
    }
    void RunAllTask()
    {
        std::vector<Functor> functor;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _tasks.swap(functor);
        }
        for (auto &task : functor)
        {
            task();
        }
        return;
    }

public:
    EventLoop() : _event_fd(CreateEventFd()), _event_channel(new Channel(this, _event_fd)), _thread_id(std::this_thread::get_id()), _timer_wheel(this)
    {
        _event_channel->SetReadCallback(std::bind(&EventLoop::ReadEventFd, this));
        _event_channel->EnableRead();
    }
    void RunInLoop(const Functor &cb)
    {
        if (InLoop())
        {
            return cb();
        }
        return QueueInLoop(cb);
    }
    void QueueInLoop(const Functor &cb)
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _tasks.push_back(cb);
        }
        WakeupEventFd();
    }
    bool InLoop()
    {
        return (_thread_id == std::this_thread::get_id());
    }
    void UpdateEvent(Channel *channel)
    {
        _poller.UpdateEvent(channel);
    }
    void RemoveEvent(Channel *channel)
    {
        _poller.RemoveEvent(channel);
    }
    void Start()
    {
        std::vector<Channel *> actives;
        _poller.Poll(&actives);
        for (auto &channel : actives)
        {
            channel->HandleEvent();
        }
        RunAllTask();
    }
    void TimerAdd(const uint64_t &id, const uint32_t &delay, const TaskFunc &cb)
    {
        _timer_wheel.TimerAdd(id, delay, cb);
    }
    void TimerRefresh(const uint64_t &id)
    {
        _timer_wheel.TimerRefresh(id);
    }
    void TimerCancel(const uint64_t &id)
    {
        _timer_wheel.TimerCancel(id);
    }
    bool HasTimer(const uint64_t &id)
    {
        return _timer_wheel.HasTimer(id);
    }

private:
    int _event_fd;
    std::vector<Functor> _tasks;
    Poller _poller;
    std::unique_ptr<Channel> _event_channel;
    std::thread::id _thread_id;
    std::mutex _mutex;
    TimerWheel _timer_wheel;
};

void Channel::Update() { _loop->UpdateEvent(this); }
void Channel::Remove() { _loop->RemoveEvent(this); }

void TimerWheel::TimerAdd(const uint64_t &id, const uint32_t &delay, const TaskFunc &cb)
{
    _loop->RunInLoop(std::bind(&TimerWheel::TimerAddInLoop, this, id, delay, cb));
}
void TimerWheel::TimerRefresh(const uint64_t &id)
{
    _loop->RunInLoop(std::bind(&TimerWheel::TimerRefreshInLoop, this, id));
}
void TimerWheel::TimerCancel(const uint64_t &id)
{
    _loop->RunInLoop(std::bind(&TimerWheel::TimerCancelInLoop, this, id));
}
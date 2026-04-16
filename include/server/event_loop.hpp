#ifndef __M_EVENT_LOOP_H__
#define __M_EVENT_LOOP_H__
#include "common/common.hpp"
#include "common/logger.hpp"
#include "poller.hpp"
#include "timer.hpp"

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
        while (true)
        {
            std::vector<Channel *> actives;
            _poller.Poll(&actives);
            for (auto &channel : actives)
            {
                channel->HandleEvent();
            }
            RunAllTask();
        }
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

    void AssertInLoop()
    {
        assert(_thread_id == std::this_thread::get_id());
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

#endif
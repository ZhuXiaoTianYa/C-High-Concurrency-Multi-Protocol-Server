#ifndef __M_TIMER_H__
#define __M_TIMER_H__
#include "common/common.hpp"
#include "common/logger.hpp"
#include "channel.hpp"

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
        {
            _task_cb();
        }
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
    uint64_t ReadTimerfd()
    {
        uint64_t val = 0;
        int ret = read(_timerfd, &val, 8);
        if (ret < 0)
        {
            FAT_LOG("READ TIMERFD FAILED!");
            abort();
        }
        return val;
    }
    void OnTime()
    {
        uint64_t count = ReadTimerfd();
        for (int i = 0; i < count; i++)
        {
            RunTimerTask();
        }
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
        if (pt)
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
        {
            _timers.erase(it);
        }
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

#endif
#ifndef __M_THREAD_H__
#define __M_THREAD_H__
#include "common/common.hpp"
#include "event_loop.hpp"

class LoopThread
{
public:
    LoopThread() : _loop(nullptr), _thread(&LoopThread::ThreadEntry, this)
    {
    }
    EventLoop *GetLoop()
    {
        EventLoop *loop;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cond.wait(lock, [&]()
                       { return _loop != nullptr; });
        }
        loop = _loop;
        return loop;
    }

private:
    void ThreadEntry()
    {
        EventLoop loop;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loop = &loop;
            _cond.notify_all();
        }
        _loop->Start();
    }

private:
    std::thread _thread;
    EventLoop *_loop;
    std::mutex _mutex;
    std::condition_variable _cond;
};

class LoopThreadPool
{
public:
    LoopThreadPool(EventLoop *loop) : _baseloop(loop), _thread_count(0), _next_loop_idx(0)
    {
    }
    void SetThreadCount(const int count)
    {
        _thread_count = count;
    }
    void Create()
    {
        if (_thread_count > 0)
        {
            _threads.resize(_thread_count);
            _loops.resize(_thread_count);
            for (int i = 0; i < _thread_count; i++)
            {
                _threads[i] = new LoopThread();
                _loops[i] = _threads[i]->GetLoop();
            }
        }
        return;
    }
    EventLoop *NextLoop()
    {
        if (_thread_count == 0)
        {
            return _baseloop;
        }
        _next_loop_idx = (_next_loop_idx + 1) % _thread_count;
        return _loops[_next_loop_idx];
    }

private:
    int _thread_count;
    int _next_loop_idx;
    EventLoop *_baseloop;
    std::vector<LoopThread *> _threads;
    std::vector<EventLoop *> _loops;
};

#endif
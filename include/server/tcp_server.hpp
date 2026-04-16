#ifndef __M_TCP_SERVER_H__
#define __M_TCP_SERVER_H__
#include "common/common.hpp"
#include "event_loop.hpp"
#include "acceptor.hpp"
#include "connection.hpp"
#include "thread.hpp"

class TcpServer
{
    using ConnectedCallback = std::function<void(const PtrConnection &)>;
    using MessageCallback = std::function<void(const PtrConnection &, Buffer *)>;
    using CloseCallback = std::function<void(const PtrConnection &)>;
    using AnyEventCallback = std::function<void(const PtrConnection &)>;
    using Functor = std::function<void()>;

public:
    TcpServer(const int port)
        : _port(port), _acceptor(&_baseloop, port), _enable_inactive_release(false), _next_id(0), _timeout(0), _pool(&_baseloop)
    {
        _acceptor.SetAcceptCallback(std::bind(&TcpServer::NewConntion, this, std::placeholders::_1));
        _acceptor.Listen();
    }
    void SetConnectedCallback(const ConnectedCallback &cb)
    {
        _connected_callback = cb;
    }
    void SetMessageCallback(const MessageCallback &cb)
    {
        _message_callback = cb;
    }
    void SetClosedCallback(const CloseCallback &cb)
    {
        _closed_callback = cb;
    }
    void SetAnyEventCallback(const AnyEventCallback &cb)
    {
        _event_callback = cb;
    }
    void SetThreadCount(const int count)
    {
        _pool.SetThreadCount(count);
    }
    void EnableInactiveRelease(const uint32_t timeout)
    {
        _timeout = timeout;
        _enable_inactive_release = true;
    }
    void RunAfter(const Functor &task, const uint32_t delay)
    {
        _baseloop.RunInLoop(std::bind(&TcpServer::RunAfterInLoop, this, task, delay));
    }

    void RemoveConntion(const PtrConnection &conn)
    {
        _baseloop.RunInLoop(std::bind(&TcpServer::RemoveConntionInLoop, this, conn));
    }
    void Start()
    {
        _pool.Create();
        _baseloop.Start();
    }

private:
    void NewConntion(const int fd)
    {
        _next_id++;
        PtrConnection conn(new Connection(_pool.NextLoop(), _next_id, fd));
        conn->SetConnectedCallback(_connected_callback);
        conn->SetMessageCallback(_message_callback);
        conn->SetAnyEventCallback(_event_callback);
        conn->SetClosedCallback(_closed_callback);
        conn->SetSrvClosedCallback(std::bind(&TcpServer::RemoveConntion, this, std::placeholders::_1));
        if (_enable_inactive_release)
            conn->EnableInactiveRelease(_timeout);
        conn->Established();
        _conns.insert(std::make_pair(_next_id, conn));
    }
    void RemoveConntionInLoop(const PtrConnection &conn)
    {
        auto it = _conns.find(conn->Id());
        if (it != _conns.end())
        {
            _conns.erase(it);
            return;
        }
    }
    void RunAfterInLoop(const Functor &task, const uint32_t delay)
    {
        _next_id++;
        _baseloop.TimerAdd(_next_id, delay, task);
    }

private:
    ConnectedCallback _connected_callback;
    MessageCallback _message_callback;
    CloseCallback _closed_callback;
    AnyEventCallback _event_callback;

private:
    int _port;
    EventLoop _baseloop;
    Acceptor _acceptor;
    uint64_t _next_id;
    uint32_t _timeout;
    bool _enable_inactive_release;
    LoopThreadPool _pool;
    std::unordered_map<uint64_t, PtrConnection> _conns;
};

#endif
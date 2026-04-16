#ifndef __M_ACCEPTOR_H__
#define __M_ACCEPTOR_H__
#include "common/common.hpp"
#include "event_loop.hpp"
#include "socket.hpp"

class Acceptor
{
    using AcceptCallback = std::function<void(int)>;

private:
    Socket _socket;
    EventLoop *_loop;
    Channel _channel;
    AcceptCallback _accept_callback;

private:
    void HandleRead()
    {
        int newfd = _socket.Accept();
        if (newfd < 0)
            return;
        if (_accept_callback)
            _accept_callback(newfd);
    }
    int CreateServer(const int &port)
    {
        bool ret = _socket.CreateServer(port);
        assert(ret == true);
        return _socket.Fd();
    }

public:
    Acceptor(EventLoop *loop, const int &port)
        : _socket(CreateServer(port)), _loop(loop), _channel(_loop, _socket.Fd())
    {
        _channel.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
    }
    void SetAcceptCallback(const AcceptCallback &cb)
    {
        _accept_callback = cb;
    }
    void Listen()
    {
        _channel.EnableRead();
    }
};

#endif
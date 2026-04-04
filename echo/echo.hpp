#pragma once
#include "../source/server.hpp"

class EchoServer
{
private:
    void OnConn(const PtrConnection &conn)
    {
        DBG_LOG("NEW CONNECTION:%p", conn.get());
    }
    void OnMsg(const PtrConnection &conn, Buffer *buf)
    {
        conn->Send(buf->ReadPosition(), buf->ReadAbleSize());
        buf->MoveReadOffset(buf->ReadAbleSize());
        // conn->Shutdown();
    }
    void OnClose(const PtrConnection &conn)
    {
        DBG_LOG("CLOSE CONNECTION:%p", conn.get());
    }

public:
    EchoServer(const int port) : _server(port)
    {
        _server.SetThreadCount(2);
        _server.EnableInactiveRelease(5);
        _server.SetConnectedCallback(std::bind(&EchoServer::OnConn, this, std::placeholders::_1));
        _server.SetMessageCallback(std::bind(&EchoServer::OnMsg, this, std::placeholders::_1, std::placeholders::_2));
        _server.SetClosedCallback(std::bind(&EchoServer::OnClose, this, std::placeholders::_1));
    }
    void Start()
    {
        _server.Start();
    }

private:
    TcpServer _server;
};
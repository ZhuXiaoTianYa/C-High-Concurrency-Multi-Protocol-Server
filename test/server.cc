#include "../source/server.hpp"

void OnClose(const PtrConnection &conn)
{
    DBG_LOG("CLOSE CONNECTION:%p", conn.get());
}

void OnMsg(const PtrConnection &conn, Buffer *buf)
{
    DBG_LOG("%s", buf->ReadPosition());
    buf->MoveReadOffset(buf->ReadAbleSize());
    std::string str = "nihao";
    conn->Send(str.c_str(), str.size());
    // conn->Shutdown();
}
void OnConn(const PtrConnection &conn)
{
    DBG_LOG("NEW CONNECTION:%p", conn.get());
}

int main()
{
    TcpServer server(8084);
    server.SetThreadCount(2);
    // server.EnableInactiveRelease(10);
    server.SetConnectedCallback(OnConn);
    server.SetMessageCallback(OnMsg);
    server.SetClosedCallback(OnClose);
    server.Start();
    return 0;
}
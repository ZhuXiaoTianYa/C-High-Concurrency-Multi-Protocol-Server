#include "../source/server.hpp"

std::unordered_map<uint64_t, PtrConnection> _conns;
EventLoop base_loop;
LoopThreadPool *_loop_pool;
int next_loop = 0;

void OnClose(const PtrConnection &conn)
{
    // std::cout << "close: " << channel->Fd() << std::endl;
    DBG_LOG("CLOSE CONNECTION:%p", conn.get());
    _conns.erase(conn->Id());
    // DBG_LOG("close: %d", channel->Fd());
    // channel->Remove();
    // delete channel;
}

void OnMsg(const PtrConnection &conn, Buffer *buf)
{
    DBG_LOG("%s", buf->ReadPosition());
    buf->MoveReadOffset(buf->ReadAbleSize());
    std::string str = "nihao";
    conn->Send(str.c_str(), str.size());
    conn->Shutdown();
}
void OnConn(const PtrConnection &conn)
{
    DBG_LOG("NEW CONNECTION:%p", conn.get());
}

// void HandleError(Channel *channel)
// {
//     HandleClose(channel);
// }
void HandleEvent(EventLoop *loop, Channel *channel, uint64_t id)
{
    // std::cout << "有了一个事件\n";
    // DBG_LOG("刷新活跃度");
    loop->TimerRefresh(id);
}
uint64_t conn_id = 0;
void NewConnection(int newfd)
{
    conn_id++;
    PtrConnection conn(new Connection(_loop_pool->NextLoop(), conn_id, newfd));
    conn->SetConnectedCallback(OnConn);
    conn->SetMessageCallback(OnMsg);
    conn->SetClosedCallback(OnClose);
    conn->EnableInactiveRelease(10);
    conn->Established();
    _conns.insert(std::make_pair(conn_id, conn));
    DBG_LOG("NEW-------");
}

int main()
{
    _loop_pool = new LoopThreadPool(&base_loop);
    _loop_pool->SetThreadCount(2);
    _loop_pool->Create();
    Acceptor acceptor(&base_loop, 8084);
    acceptor.SetAcceptCallback(std::bind(NewConnection, std::placeholders::_1));
    acceptor.Listen();
    base_loop.Start();
    // Socket server;
    // server.CreateServer(8084);
    // while (true)
    // {
    //     int newfd = server.Accept();
    //     if (newfd == -1)
    //     {
    //         ERR_LOG("ACCEPT FAILED!");
    //         continue;
    //     }
    //     Socket cli(newfd);
    //     char buf[1024] = {0};
    //     int ret = cli.Recv(buf, 1023);
    //     if (ret < 0)
    //     {
    //         cli.Close();
    //         continue;
    //     }
    //     cli.Send(buf, ret);
    //     cli.Close();
    // }
    // server.Close();
    return 0;
}
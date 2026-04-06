#include "../source/server.hpp"

int main()
{
    std::string str = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    str += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    str += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    str += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    str += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    Socket cli;
    cli.CreateClient(8084, "127.0.0.1");
    for (;;)
    {
        char buf[1024] = {0};
        cli.Send(str.c_str(), str.size());
        cli.Recv(buf, 1023);
        DBG_LOG("%s", buf);
        sleep(3);
    }
    return 0;
}
#include "../source/server.hpp"

int main()
{
    std::string str = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    for (int i = 0; i < 100; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            return -1;
        }
        if (pid == 0)
        {
            Socket cli;
            cli.CreateClient(8084, "127.0.0.1");
            while (1)
            {
                char buf[1024] = {0};
                cli.Send(str.c_str(), str.size());
                cli.Recv(buf, 1023);
                DBG_LOG("%s", buf);
                sleep(1);
            }
        }
    }
    while (1)
        sleep(1);
    return 0;
}
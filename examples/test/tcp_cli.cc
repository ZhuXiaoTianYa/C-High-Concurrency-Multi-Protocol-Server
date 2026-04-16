#include "../source/server.hpp"

int main()
{
    std::string str = "hahahaha";
    Socket cli;
    cli.CreateClient(8084, "127.0.0.1");
    for (int i = 0; i < 5; i++)
    {
        char buf[1024] = {0};
        cli.Send(str.c_str(), str.size());
        cli.Recv(buf, 1023);
        DBG_LOG("%s", buf);
        sleep(1);
    }
    while (1)
    {
        sleep(1);
    }
    return 0;
}
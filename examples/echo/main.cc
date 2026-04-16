#include "echo.hpp"

int main()
{
    EchoServer server(8084);
    server.Start();
    return 0;
}
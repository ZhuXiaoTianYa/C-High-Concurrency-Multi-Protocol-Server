#include "../http/http.hpp"

int main()
{
    std::string str = "PUT /123.txt HTTP/1.1\r\nConnection: keep-alive\r\n";
    std::string body;
    Util::ReadFile("./hello.txt", &body);
    std::string content_len = std::to_string(body.size());
    str += ("Content-Length: " + content_len + "\r\n\r\n");
    DBG_LOG("%s", content_len.c_str());
    Socket cli;
    cli.CreateClient(8084, "127.0.0.1");
    char buf[1024] = {0};
    cli.Send(str.c_str(), str.size());
    cli.Send(body.c_str(), body.size());
    cli.Recv(buf, 1023);
    DBG_LOG("%s", buf);
    return 0;
}
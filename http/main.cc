#include "http.hpp"

#define DEFAULT_BASE_DIR "./wwwroot"

std::string RequestStr(const HttpRequest &req)
{
    std::stringstream rsp_str;
    rsp_str << req._method << " " << req._path << " " << req._version << "\r\n";
    for (auto &head : req._params)
    {
        rsp_str << head.first << ": " << head.second << "\r\n";
    }
    for (auto &head : req._headers)
    {
        rsp_str << head.first << ": " << head.second << "\r\n";
    }
    rsp_str << "\r\n";
    rsp_str << req._body;
    return rsp_str.str();
}

void Hello(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
void Login(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
void PutFile(const HttpRequest &req, HttpResponse *rsp)
{
    Util::WriteFile(std::string(DEFAULT_BASE_DIR + req._path), req._body);
}
void DelFile(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}

int main()
{
    HttpServer server(8084);
    server.SetBaseDir(DEFAULT_BASE_DIR);
    server.SetThreadCount(3);
    server.Get("/hello", Hello);
    server.Post("/login", Login);
    server.Put("/123.txt", PutFile);
    server.Delete("/123.txt", DelFile);
    server.Listen();
    return 0;
}
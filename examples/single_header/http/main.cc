#include "http.hpp"

int main()
{
    HttpServer server(8080);
    server.SetThreadCount(2);
    server.SetBaseDir("./wwwroot");
    server.Get("/", [](const HttpRequest &req, HttpResponse *rsp) {
        rsp->SetContent("<h1>Hello, HTTP Server!</h1>", "text/html");
    });
    server.Get("/hello", [](const HttpRequest &req, HttpResponse *rsp) {
        std::string name = req.GetParam("name");
        if (name.empty()) {
            name = "World";
        }
        rsp->SetContent("<h1>Hello, " + name + "!</h1>", "text/html");
    });
    server.Listen();
    return 0;
}
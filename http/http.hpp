#ifndef __M_HTTP_H__
#define __M_HTTP_H__
#include "../source/server.hpp"
#include <regex>
#include <fstream>
#include <sys/stat.h>

class Util
{
public:
    static size_t Split(const std::string &src, const std::string &sep, std::vector<std::string> *arry)
    {
        size_t offset = 0;
        while (offset < src.size())
        {
            size_t pos = src.find(sep, offset);
            if (pos == std::string::npos)
            {
                arry->push_back(src.substr(offset));
                return arry->size();
            }
            if (pos - offset > 0)
            {
                arry->push_back(src.substr(offset, pos - offset));
            }
            offset = pos + 1;
        }
        return arry->size();
    }
    static bool ReadFile(const std::string &filename, std::string *buf)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false)
        {
            ERR_LOG("OPEN %s FILE FAILED!", filename.c_str());
            return false;
        }
        size_t fsize = 0;
        ifs.seekg(0, ifs.end);
        fsize = ifs.tellg();
        ifs.seekg(0, ifs.beg);
        buf->resize(fsize);
        ifs.read(&(*buf)[0], fsize);
        if (ifs.good() == false)
        {
            ERR_LOG("READ %s FILE FAILED!", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }
    static bool WriteFile(const std::string &filename, const std::string &buf)
    {
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
        if (ofs.is_open() == false)
        {
            ERR_LOG("OPEN %s FILE FAILED!", filename.c_str());
            return false;
        }
        ofs.write(buf.c_str(), buf.size());
        if (ofs.good() == false)
        {
            ERR_LOG("WRITE %s FILE FAILED!", filename.c_str());
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
    static std::string UrlEncode(const std::string &url, bool convert_space_to_plus)
    {
        std::string res;
        for (auto &c : url)
        {
            if (c == '.' || c == '-' || c == '_' || c == '~' || isalnum(c))
            {
                res += c;
                continue;
            }
            if (c == ' ' && convert_space_to_plus)
            {
                res += '+';
                continue;
            }
            char tmp[4] = {0};
            snprintf(tmp, 4, "%%%02X", c);
            res += tmp;
        }
        return res;
    }
    static char HexToI(const char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        else if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        else
            return -1;
    }
    static std::string UrlDecode(const std::string &url, bool convert_plus_to_space)
    {
        std::string res;
        for (int i = 0; i < url.size(); i++)
        {
            if (url[i] == '+' && convert_plus_to_space == true)
            {
                res += ' ';
                continue;
            }
            if (url[i] == '%' && (i + 2) < url.size())
            {
                char v1 = HexToI(url[i + 1]);
                char v2 = HexToI(url[i + 2]);
                res += (v1 << 4 + v2);
                i += 2;
                continue;
            }
            res += url[i];
        }
        return res;
    }
    static bool IsDirectory(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }
    static bool IsRegular(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISREG(st.st_mode);
    }
    static bool ValidPath(const std::string &filename)
    {
        int level = 0;
        std::vector<std::string> subdir;
        Split(filename, "/", &subdir);
        for (auto &dir : subdir)
        {
            if (dir == "..")
            {
                level--;
                if (level < 0)
                    return false;
                continue;
            }
            level++;
        }
        return true;
    }
    static std::string StatusDesc(const int status)
    {
        static std::unordered_map<int, std::string> _status_msg = {
            {100, "Continue"},
            {101, "Switching Protocol"},
            {102, "Processing"},
            {103, "Early Hints"},
            {200, "OK"},
            {201, "Created"},
            {202, "Accepted"},
            {203, "Non-Authoritative Information"},
            {204, "No Content"},
            {205, "Reset Content"},
            {206, "Partial Content"},
            {207, "Multi-Status"},
            {208, "Already Reported"},
            {226, "IM Used"},
            {300, "Multiple Choice"},
            {301, "Moved Permanently"},
            {302, "Found"},
            {303, "See Other"},
            {304, "Not Modified"},
            {305, "Use Proxy"},
            {306, "unused"},
            {307, "Temporary Redirect"},
            {308, "Permanent Redirect"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {402, "Payment Required"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {406, "Not Acceptable"},
            {407, "Proxy Authentication Required"},
            {408, "Request Timeout"},
            {409, "Conflict"},
            {410, "Gone"},
            {411, "Length Required"},
            {412, "Precondition Failed"},
            {413, "Payload Too Large"},
            {414, "URI Too Long"},
            {415, "Unsupported Media Type"},
            {416, "Range Not Satisfiable"},
            {417, "Expectation Failed"},
            {418, "I'm a teapot"},
            {421, "Misdirected Request"},
            {422, "Unprocessable Entity"},
            {423, "Locked"},
            {424, "Failed Dependency"},
            {425, "Too Early"},
            {426, "Upgrade Required"},
            {428, "Precondition Required"},
            {429, "Too Many Requests"},
            {431, "Request Header Fields Too Large"},
            {451, "Unavailable For Legal Reasons"},
            {501, "Not Implemented"},
            {502, "Bad Gateway"},
            {503, "Service Unavailable"},
            {504, "Gateway Timeout"},
            {505, "HTTP Version Not Supported"},
            {506, "Variant Also Negotiates"},
            {507, "Insufficient Storage"},
            {508, "Loop Detected"},
            {510, "Not Extended"},
            {511, "Network Authentication Required"}};
        auto it = _status_msg.find(status);
        if (it != _status_msg.end())
        {
            return it->second;
        }
        return "UnKnow";
    }
    static std::string ExtMime(const std::string filename)
    {
        std::unordered_map<std::string, std::string> _mime_msg = {
            {".aac", "audio/aac"},
            {".abw", "application/x-abiword"},
            {".arc", "application/x-freearc"},
            {".avi", "video/x-msvideo"},
            {".azw", "application/vnd.amazon.ebook"},
            {".bin", "application/octet-stream"},
            {".bmp", "image/bmp"},
            {".bz", "application/x-bzip"},
            {".bz2", "application/x-bzip2"},
            {".csh", "application/x-csh"},
            {".css", "text/css"},
            {".csv", "text/csv"},
            {".doc", "application/msword"},
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {".eot", "application/vnd.ms-fontobject"},
            {".epub", "application/epub+zip"},
            {".gif", "image/gif"},
            {".htm", "text/html"},
            {".html", "text/html"},
            {".ico", "image/vnd.microsoft.icon"},
            {".ics", "text/calendar"},
            {".jar", "application/java-archive"},
            {".jpeg", "image/jpeg"},
            {".jpg", "image/jpeg"},
            {".js", "text/javascript"},
            {".json", "application/json"},
            {".jsonld", "application/ld+json"},
            {".mid", "audio/midi"},
            {".midi", "audio/x-midi"},
            {".mjs", "text/javascript"},
            {".mp3", "audio/mpeg"},
            {".mpeg", "video/mpeg"},
            {".mpkg", "application/vnd.apple.installer+xml"},
            {".odp", "application/vnd.oasis.opendocument.presentation"},
            {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
            {".odt", "application/vnd.oasis.opendocument.text"},
            {".oga", "audio/ogg"},
            {".ogv", "video/ogg"},
            {".ogx", "application/ogg"},
            {".otf", "font/otf"},
            {".png", "image/png"},
            {".pdf", "application/pdf"},
            {".ppt", "application/vnd.ms-powerpoint"},
            {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
            {".rar", "application/x-rar-compressed"},
            {".rtf", "application/rtf"},
            {".sh", "application/x-sh"},
            {".svg", "image/svg+xml"},
            {".swf", "application/x-shockwave-flash"},
            {".tar", "application/x-tar"},
            {".tif", "image/tiff"},
            {".tiff", "image/tiff"},
            {".ttf", "font/ttf"},
            {".txt", "text/plain"},
            {".vsd", "application/vnd.visio"},
            {".wav", "audio/wav"},
            {".weba", "audio/webm"},
            {".webm", "video/webm"},
            {".webp", "image/webp"},
            {".woff", "font/woff"},
            {".woff2", "font/woff2"},
            {".xhtml", "application/xhtml+xml"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {".xml", "application/xml"},
            {".xul", "application/vnd.mozilla.xul+xml"},
            {".zip", "application/zip"},
            {".3gp", "video/3gpp"},
            {".3g2", "video/3gpp2"},
            {".7z", "application/x-7z-compressed"}};
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream";
        }
        std::string ext = filename.substr(pos);
        auto it = _mime_msg.find(ext);
        if (it == _mime_msg.end())
        {
            return "application/octet-stream";
        }
        return it->second;
    }
};

class HttpRequest
{
public:
    std::string _method;
    std::string _path;
    std::string _version;
    std::string _body;
    std::unordered_map<std::string, std::string> _headers;
    std::unordered_map<std::string, std::string> _params;
    std::smatch _matches;

public:
    void Reset()
    {
        _method.clear();
        _path.clear();
        _version.clear();
        _body.clear();
        _headers.clear();
        _params.clear();
        std::smatch matches;
        matches.swap(_matches);
    }
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert(std::make_pair(key, val));
    }
    bool HasHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return false;
        }
        return true;
    }
    std::string GetHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    void SetParam(const std::string &key, const std::string &val)
    {
        _params.insert(std::make_pair(key, val));
    }
    bool HasParam(const std::string &key)
    {
        auto it = _params.find(key);
        if (it == _params.end())
        {
            return false;
        }
        return true;
    }
    std::string GetParam(const std::string &key)
    {
        auto it = _params.find(key);
        if (it == _params.end())
        {
            return "";
        }
        return it->second;
    }
    size_t ContentLength()
    {
        bool ret = HasHeader("Content-Length");
        if (ret == false)
        {
            return 0;
        }
        std::string clen = GetHeader("Content-Length");
        return std::stol(clen);
    }
    bool Close() const
    {
        if (HasHeader("Connection") == true && (GetHeader("Connection") == "keep-alive" || GetHeader("Connection") == "Keep-Alive"))
        {
            return false;
        }
        return true;
    }
};

class HttpResponse
{
public:
    HttpResponse() : _status(200), _redirect_flag(false) {}
    HttpResponse(const int status) : _status(status), _redirect_flag(false) {}
    void Reset()
    {
        _status = 200;
        _redirect_flag = false;
        _body.clear();
        _redirect_url.clear();
        _headers.clear();
    }
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert(std::make_pair(key, val));
    }
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return false;
        }
        return true;
    }
    std::string GetHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    void SetContent(const std::string &body, const std::string &type)
    {
        _body = body;
        SetHeader("Content-Type", type);
    }
    void SetRedirect(const std::string &url, int status = 302)
    {
        _redirect_url = true;
        _status = status;
        _redirect_url = url;
    }
    bool Close()
    {
        if (HasHeader("Connection") == true && (GetHeader("Connection") == "keep-alive" || GetHeader("Connection") == "Keep-Alive"))
        {
            return false;
        }
        return true;
    }

public:
    int _status;
    bool _redirect_flag;
    std::string _body;
    std::string _redirect_url;
    std::unordered_map<std::string, std::string> _headers;
};

enum class HttpRecvStatus
{
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER,
    RECV_HTTP_ERROR
};

#define MAX_LINE 8192
class HttpContext
{
public:
    HttpContext() : _resp_status(200), _recv_status(HttpRecvStatus::RECV_HTTP_LINE) {}
    int RespStatus()
    {
        return _resp_status;
    }
    HttpRequest &Request()
    {
        return _request;
    }
    HttpRecvStatus RecvStatus()
    {
        return _recv_status;
    }
    void RecvHttpRequest(Buffer *buf)
    {
        switch (_recv_status)
        {
        case HttpRecvStatus::RECV_HTTP_LINE:
            RecvHttpLine(buf);
        case HttpRecvStatus::RECV_HTTP_HEAD:
            RecvHttpHead(buf);
        case HttpRecvStatus::RECV_HTTP_BODY:
            RecvHttpBody(buf);
        }
        return;
    }
    void Reset()
    {
        _resp_status = 200;
        _recv_status = HttpRecvStatus::RECV_HTTP_LINE;
        _request.Reset();
    }

private:
    bool RecvHttpLine(Buffer *buf)
    {
        if (_recv_status != HttpRecvStatus::RECV_HTTP_LINE)
            return false;
        std::string line = buf->GetLineAndMove();
        if (line.size() == 0)
        {
            if (buf->ReadAbleSize() > MAX_LINE)
            {
                _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
                _resp_status = 414;
                return false;
            }
            return false;
        }
        if (line.size() > MAX_LINE)
        {
            _resp_status = 414;
            _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
            return false;
        }
        bool ret = ParseHttpLine(line);
        if (ret == false)
        {
            return false;
        }
        _recv_status = HttpRecvStatus::RECV_HTTP_HEAD;
        return true;
    }
    bool ParseHttpLine(const std::string &line)
    {
        std::smatch matches;
        std::regex e("(GET|POST|HEAD|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
        int ret = std::regex_match(line, matches, e);
        if (ret == false)
        {
            _resp_status = 400;
            _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
            return false;
        }
        _request._method = matches[1];
        std::transform(_request._method.begin(), _request._method.end(), _request._method.begin(), ::toupper);
        _request._path = Util::UrlDecode(matches[2], false);
        _request._version = matches[4];
        std::vector<std::string> query_string_arry;
        Util::Split(matches[3], "&", &query_string_arry);
        for (auto &str : query_string_arry)
        {
            size_t pos = str.find('=');
            if (pos == std::string::npos)
            {
                _recv_status == HttpRecvStatus::RECV_HTTP_ERROR;
                _resp_status = 400;
                return false;
            }
            _request.SetParam(str.substr(0, pos), str.substr(pos + 1));
        }
        return true;
    }
    bool RecvHttpHead(Buffer *buf)
    {
        if (_recv_status != HttpRecvStatus::RECV_HTTP_HEAD)
            return false;
        while (true)
        {
            std::string line = buf->GetLineAndMove();
            if (line.size() == 0)
            {
                if (buf->ReadAbleSize() > MAX_LINE)
                {
                    _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
                    _resp_status = 414;
                    return false;
                }
                return false;
            }
            if (line.size() > MAX_LINE)
            {
                _resp_status = 414;
                _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
                return false;
            }
            if (line == "\n" || line == "\r\n")
            {
                break;
            }
            bool ret = ParseHttpHead(line);
            if (ret == false)
            {
                return false;
            }
        }
        _recv_status = HttpRecvStatus::RECV_HTTP_BODY;
        return true;
    }
    bool ParseHttpHead(std::string &line)
    {
        if (line.back() == '\n')
            line.pop_back();
        if (line.back() == '\r')
            line.pop_back();
        size_t pos = line.find(": ");
        if (pos == std::string::npos)
        {
            _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
            _resp_status = 400;
            return false;
        }
        _request.SetHeader(line.substr(0, pos), line.substr(pos + 2));
        return true;
    }
    bool RecvHttpBody(Buffer *buf)
    {
        if (_recv_status != HttpRecvStatus::RECV_HTTP_BODY)
            return false;
        size_t content_length = _request.ContentLength();
        if (content_length == 0)
        {
            _recv_status = HttpRecvStatus::RECV_HTTP_OVER;
            return true;
        }
        size_t real_len = content_length - _request._body.size();
        if (real_len <= buf->ReadAbleSize())
        {
            _request._body.append(buf->ReadPosition(), real_len);
            buf->MoveReadOffset(real_len);
            _recv_status = HttpRecvStatus::RECV_HTTP_OVER;
            return true;
        }
        _request._body.append(buf->ReadPosition(), buf->ReadAbleSize());
        buf->MoveReadOffset(buf->ReadAbleSize());
        return true;
    }

private:
    int _resp_status;
    HttpRecvStatus _recv_status;
    HttpRequest _request;
};
#define DEFAULT_TIMEOUT 10
class HttpServer
{
    using Handler = std::function<void(const HttpRequest &, HttpResponse *)>;
    using Handlers = std::vector<std::pair<std::regex, Handler>>;

public:
    HttpServer(const int port, const uint32_t timeout = DEFAULT_TIMEOUT)
        : _server(port)
    {
        _server.SetConnectedCallback(std::bind(&HttpServer::OnConnected, this, std::placeholders::_1));
        _server.SetMessageCallback(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        _server.EnableInactiveRelease(timeout);
    }
    void Get(const std::string &pattern, const Handler &handler)
    {
        _get_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Post(const std::string &pattern, const Handler &handler)
    {
        _post_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Put(const std::string &pattern, const Handler &handler)
    {
        _put_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Delete(const std::string &pattern, const Handler &handler)
    {
        _delete_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void SetBaseDir(const std::string path)
    {
        assert(Util::IsDirectory(path) == true);
        _basedir = path;
    }
    void SetThreadCount(const int count)
    {
        _server.SetThreadCount(count);
    }
    void Listen()
    {
        _server.Start();
    }

private:
    void OnConnected(const PtrConnection &conn)
    {
        conn->SetContext(HttpContext());
        DBG_LOG("NEW CONNECTION %p", conn.get());
    }
    void OnMessage(const PtrConnection &conn, Buffer *buf)
    {
        while (buf->ReadAbleSize() > 0)
        {
            HttpContext *context = std::any_cast<HttpContext>(conn->GetContext());
            context->RecvHttpRequest(buf);
            HttpRequest &req = context->Request();
            HttpResponse rsp(context->RespStatus());
            if (context->RespStatus() >= 400)
            {
                ErrorHandler(req, &rsp);
                WriteResponse(conn, req, &rsp);
                buf->MoveReadOffset(buf->ReadAbleSize());
                context->Reset();
                conn->Shutdown();
                return;
            }
            if (context->RecvStatus() != HttpRecvStatus::RECV_HTTP_OVER)
            {
                return;
            }
            Route(req, &rsp);
            WriteResponse(conn, req, &rsp);
            context->Reset();
            if (rsp.Close() == true)
            {
                conn->Shutdown();
                break;
            }
        }
        return;
    }
    void Route(HttpRequest &req, HttpResponse *rsp)
    {
        if (IsFileHandler(req) == true)
        {
            return FileHandler(req, rsp);
        }
        if (req._method == "GET" || req._method == "HEAD")
        {
            return Dispatcher(req, rsp, _get_route);
        }
        else if (req._method == "POST")
        {
            return Dispatcher(req, rsp, _post_route);
        }
        else if (req._method == "PUT")
        {
            return Dispatcher(req, rsp, _put_route);
        }
        else if (req._method == "DELETE")
        {
            return Dispatcher(req, rsp, _delete_route);
        }
        rsp->_status = 405;
        return;
    }
    void Dispatcher(HttpRequest &req, HttpResponse *rsp, Handlers headers)
    {
        for (auto &handler : headers)
        {
            const std::regex &re = handler.first;
            if (std::regex_match(req._path, req._matches, re) == false)
            {
                continue;
            }
            return handler.second(req, rsp);
        }
        rsp->_status = 404;
        return;
    }
    void ErrorHandler(const HttpRequest &req, HttpResponse *rsp)
    {
        std::string body = "<!DOCTYPEhtml><html><head><metahttp-equiv=\"Content-Type\"content=\"text/html;charset=utf-8\"><title>简单页面</title></head><body><h1>这是我用h1显示的一段话</h1></body></html>";
        rsp->SetContent(body, "text/html");
    }
    void WriteResponse(const PtrConnection &conn, const HttpRequest &req, HttpResponse *rsp)
    {
        if (req.Close() == true)
        {
            rsp->SetHeader("Connection", "close");
        }
        else if (req.Close() == false && rsp->HasHeader("Connection") == false)
        {
            rsp->SetHeader("Connection", "keep-alive");
        }
        if (rsp->_body.empty() == false && rsp->HasHeader("Content-Length") == false)
        {
            rsp->SetHeader("Content-Length", std::to_string(rsp->_body.size()));
        }
        if (rsp->_body.empty() == false && rsp->HasHeader("Content-Type") == false)
        {
            rsp->SetHeader("Content-Type", "application/octet-stream");
        }
        if (rsp->_redirect_flag == true)
        {
            rsp->SetHeader("Location", rsp->_redirect_url);
        }
        std::stringstream rsp_str;
        rsp_str << req._version << " " << rsp->_status << " " << Util::StatusDesc(rsp->_status) << "\r\n";
        for (auto &head : rsp->_headers)
        {
            rsp_str << head.first << ": " << head.second << "\r\n";
        }
        rsp_str << "\r\n";
        rsp_str << rsp->_body;
        conn->Send(rsp_str.str().c_str(), rsp_str.str().size());
    }
    bool IsFileHandler(HttpRequest &req)
    {
        if (_basedir.empty())
        {
            return false;
        }
        if (req._method != "GET" && req._method != "HEAD")
        {
            return false;
        }
        if (Util::ValidPath(req._path) == false)
        {
            return false;
        }
        std::string req_path = _basedir + req._path;
        if (req_path.back() = '/')
        {
            req_path += "index.html";
        }
        if (Util::IsRegular(req_path) == false)
        {
            return false;
        }
        return true;
    }
    void FileHandler(const HttpRequest &req, HttpResponse *rsp)
    {
        std::string req_path = _basedir + req._path;
        if (req_path.back() = '/')
        {
            req_path += "index.html";
        }
        bool ret = Util::ReadFile(req_path, &rsp->_body);
        if (ret == false)
        {
            return;
        }
        std::string mime = Util::ExtMime(req_path);
        rsp->SetHeader("Content-Type", mime);
        return;
    }

private:
    Handlers _get_route;
    Handlers _post_route;
    Handlers _put_route;
    Handlers _delete_route;
    std::string _basedir;
    TcpServer _server;
};

#endif
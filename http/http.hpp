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
    bool Close()
    {
        if (HasHeader("Connection") == true && (GetHeader("Connection") == "keep-alive" || GetHeader("Connection") == "Keep-Alive"))
        {
            return true;
        }
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
            return true;
        }
    }

private:
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
        std::regex e("(GET|POST|HEAD|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?");
        int ret = std::regex_match(line, matches, e);
        if (ret == false)
        {
            _resp_status = 400;
            _recv_status = HttpRecvStatus::RECV_HTTP_ERROR;
            return false;
        }
        _request._method = matches[1];
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
    bool ParseHttpHead(const std::string &line)
    {
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

#endif
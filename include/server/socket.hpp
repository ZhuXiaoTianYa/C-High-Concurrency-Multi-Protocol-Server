#ifndef __M_SOCKET_H__
#define __M_SOCKET_H__
#include "common/common.hpp"
#include "common/logger.hpp"

class Socket
{
private:
    int _sockfd;

public:
    Socket() : _sockfd(-1) {}
    Socket(const int &sockfd) : _sockfd(sockfd) {}
    ~Socket() { Close(); }
    int Fd() { return _sockfd; }
    bool Create()
    {
        int newfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newfd < 0)
        {
            ERR_LOG("CREATE SOCKET FAILED!");
            return false;
        }
        _sockfd = newfd;
        return true;
    }
    bool Bind(const std::string &ip, const uint16_t &port)
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        socklen_t len = sizeof(addr);
        int ret = bind(_sockfd, (sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("BIND SOCKET FAILED!");
            return false;
        }
        return true;
    }

    bool Listen(const int &backlog = MAX_LISTEN)
    {
        int ret = listen(_sockfd, backlog);
        if (ret == -1)
        {
            FAT_LOG("LISTEN SOCKET FAILED!");
            return false;
        }
        return true;
    }

    bool Connect(const std::string &ip, const uint16_t &port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        socklen_t len = sizeof(addr);
        int ret = connect(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("CONNECT FAILED!");
            return false;
        }
        return true;
    }

    int Accept()
    {
        int ret = accept(_sockfd, NULL, NULL);
        if (ret == -1)
        {
            ERR_LOG("ACCEPT SOCKET FAILED!");
            return ret;
        }
        return ret;
    }
    ssize_t Recv(void *buf, const size_t &len, const int &flag = 0)
    {
        ssize_t ret = recv(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            if (ret == EAGAIN || ret == EINTR)
            {
                return 0;
            }
            return -1;
        }
        return ret;
    }

    ssize_t NonBlockRecv(void *buf, const size_t &len)
    {
        return Recv(buf, len, MSG_DONTWAIT);
    }
    ssize_t Send(const void *buf, const size_t &len, const int &flag = 0)
    {
        if (len == 0)
            return 0;
        ssize_t ret = send(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            if (ret == EAGAIN || ret == EINTR)
            {
                return 0;
            }
            return -1;
        }
        return ret;
    }

    ssize_t NonBlockSend(void *buf, const size_t &len)
    {
        return Send(buf, len, MSG_DONTWAIT);
    }
    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }
    bool CreateServer(const uint16_t &port, const std::string &ip = "0.0.0.0", bool block_flag = true)
    {
        if (Create() == false)
            return false;
        ReuseAddress();
        if (Bind(ip, port) == false)
            return false;
        if (Listen() == false)
            return false;
        if (block_flag == false)
            NonBlock();
        return true;
    }
    bool CreateClient(const uint16_t &port, const std::string &ip)
    {
        if (Create() == false)
            return false;
        if (Connect(ip, port) == false)
            return false;
        return true;
    }
    void ReuseAddress()
    {
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(int));
    }
    void NonBlock()
    {
        int flag = fcntl(_sockfd, F_GETFD, 0);
        fcntl(_sockfd, F_SETFD, flag | O_NONBLOCK);
    }
};

#endif
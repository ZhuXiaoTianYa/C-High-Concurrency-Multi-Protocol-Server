#ifndef __M_POLLER_H__
#define __M_POLLER_H__
#include "common/common.hpp"
#include "common/logger.hpp"
#include "channel.hpp"

class Poller
{
private:
    void Update(Channel *channel, const int &op)
    {
        int fd = channel->Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = channel->Events();
        int ret = epoll_ctl(_epfd, op, fd, &ev);
        if (ret < 0)
        {
            ERR_LOG("EPOLL CTL FAILED!");
            return;
        }
        return;
    }
    bool HasChannel(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it == _channels.end())
        {
            return false;
        }
        return true;
    }

public:
    Poller() : _epfd(-1)
    {
        _epfd = epoll_create(1);
        if (_epfd < 0)
        {
            FAT_LOG("EPOLL CREATE FAILED");
            _epfd = -1;
            abort();
        }
    }
    void UpdateEvent(Channel *channel)
    {
        if (HasChannel(channel) == false)
        {
            _channels.insert(std::make_pair(channel->Fd(), channel));
            Update(channel, EPOLL_CTL_ADD);
        }
        Update(channel, EPOLL_CTL_MOD);
    }
    void RemoveEvent(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it != _channels.end())
        {
            _channels.erase(it);
        }
        Update(channel, EPOLL_CTL_DEL);
    }
    void Poll(std::vector<Channel *> *active)
    {
        int nfds = epoll_wait(_epfd, _evs, MAX_EPOLLEVENTS, -1);
        if (nfds < 0)
        {
            if (nfds == EINTR)
                return;
            ERR_LOG("EPOLL WAIT FAILED!");
            abort();
        }
        for (int i = 0; i < nfds; i++)
        {
            auto it = _channels.find(_evs[i].data.fd);
            assert(it != _channels.end());
            it->second->SetREvents(_evs[i].events);
            active->push_back(it->second);
        }
    }

private:
    int _epfd;
    struct epoll_event _evs[MAX_EPOLLEVENTS];
    std::unordered_map<int, Channel *> _channels;
};

#endif
#ifndef __M_NETWORK_H__
#define __M_NETWORK_H__
#include "common/common.hpp"
#include "common/logger.hpp"

class NetWork
{
public:
    NetWork()
    {
        DBG_LOG("SIGPIPE INIT");
        signal(SIGPIPE, SIG_IGN);
    }
};
static NetWork nw;

#endif
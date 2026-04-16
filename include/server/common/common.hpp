#ifndef __M_COMMON_H__
#define __M_COMMON_H__
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <cassert>
#include <string>
#include <cstring>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <any>
#include <thread>
#include <fcntl.h>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

#define BUFFER_DEFAULT_SIZE 1024
#define MAX_LISTEN 1024
#define MAX_EPOLLEVENTS 1024

#endif
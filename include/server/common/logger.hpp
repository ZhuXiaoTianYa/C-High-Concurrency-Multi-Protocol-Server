#ifndef __M_LOGGER_H__
#define __M_LOGGER_H__
#include <unistd.h>
#include <cstdarg>
#include <ctime>
#include <iostream>

#define DBG 0
#define INF 1
#define ERR 2
#define FAT 3
#define NON 4
#define LOG_LEVEL DBG

#define LOG(level, format, ...)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (level < LOG_LEVEL)                                                                                          \
            break;                                                                                                      \
        time_t t = time(NULL);                                                                                          \
        struct tm *ltm = localtime(&t);                                                                                 \
        char timer[32] = {0};                                                                                           \
        strftime(timer, 31, "%H:%M:%S", ltm);                                                                          \
        fprintf(stdout, "[%p %s %s:%d] " format "\n", (void *)pthread_self(), timer, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)
#define FAT_LOG(format, ...) LOG(FAT, format, ##__VA_ARGS__)

#endif
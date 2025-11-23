#include "debug.h"
#include <sys/time.h>

std::string getNowTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm t;
    localtime_r(&tv.tv_sec, &t);
    char buf[64] = {0};
    int ms = tv.tv_usec / 1000;
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec, ms);
    return std::string(buf);
}
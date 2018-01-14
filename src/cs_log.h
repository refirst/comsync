#ifndef CS_LOG_H
#define CS_LOG_H

#include "zookeeper/zookeeper_log.h"

#define CS_DEBUG(x) do { \
    LOG_DEBUG(x); \
} while (0)

#define CS_INFO(x) do { \
    LOG_INFO(x); \
} while (0)

#define CS_WARN(x) do { \
    LOG_WARN(x); \
} while(0)

#define CS_ERROR(x) do { \
    LOG_ERROR(x); \
} while (0)

namespace cs_log
{
    void log_init(int level, const char* file);
    void log_uninit();
    void log_reopen();
}

#endif

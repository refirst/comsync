#ifndef CS_EPOLL_H
#define CS_EPOLL_H

#include <stdint.h>

#define CS_EVENT_READ  0x001
#define CS_EVENT_WRITE 0x010
#define CS_EVENT_ERR   0x100

typedef int (*cs_event_cb)(int, uint32_t);

namespace cs_epoll
{
    void event_base_init(int size, cs_event_cb cb);
    void event_base_uninit();
    int event_add(int fd);
    int event_del(int fd);
    int event_wait(int timeout);
}

#endif

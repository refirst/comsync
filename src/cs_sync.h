#ifndef CS_SYNC_H
#define CS_SYNC_H

#include "cs_conf.h"

struct cs_param
{
    std::string _zk_hosts;
    int _zk_timeout;
    int _ev_size;
    int _ev_timeout;
};

namespace cs_sync
{
    void sync_start(struct cs_conf_info* conf);
    void sync_stop();
    void sync_reload(std::vector<struct cs_monitor_info>& infos, const unsigned backup);
    void sync_release();
    int sync_cb(int fd, uint32_t events);
    int sync_loop();
}

#endif

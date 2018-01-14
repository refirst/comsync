#ifndef CS_CONF_H
#define CS_CONF_H

#include <string>
#include <set>
#include <vector>

struct cs_monitor_info
{
    std::string _monitor_pid;
    std::set<std::string> _monitor_files;
};

struct cs_conf_info
{
    int _log_level;
    std::string _log_file;
    unsigned _need_backup;
    int _ev_size;
    int _ev_timeout;
    std::string _zk_hosts;
    int _zk_timeout;
    std::vector<struct cs_monitor_info> _monitor_infos;
};

namespace cs_conf
{
    int conf_load(struct cs_conf_info* conf, const char* path);
}

#endif

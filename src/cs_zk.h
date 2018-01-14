#ifndef CS_ZK_H
#define CS_ZK_H

#include <string>
#include <set>

namespace cs_zk
{
    void zk_init(const std::string& hosts, int timeout);
    void zk_uninit();
    void is_need_backup(unsigned backup);
    void create_pid_paths(const std::set<std::string>& pids);
    void update_monitor_path(const std::string& pid, const std::string& file_path, const int from_zk);
    void delete_monitor_path(const std::string& pid, const std::string& file_path);
}

#endif

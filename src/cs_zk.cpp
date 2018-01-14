#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "cs_util.h"
#include "cs_md5.h"
#include "cs_log.h"
#include "cs_zk.h"

const char MONITOR_BASE_PATH[] = "/commonitor";

namespace cs_zk
{
    static zhandle_t* _zk_handle = NULL;
    static volatile unsigned _notify_backup = 1;
    static void exists_watcher(zhandle_t* zh, int type, int state, const char* path, void* watcher_ctx);

    static void global_watcher(zhandle_t* zh, int type, int state, const char* path, void* watcher_ctx)
    {
        UNUSED(zh);
        UNUSED(watcher_ctx);
        if (path != NULL)
        {
            CS_INFO(("zk global watcher type[%d], state[%d], path[%s]", type, state, path));
        }
    }

    void zk_init(const std::string& hosts, int timeout)
    {
        CS_INFO(("init connect zkserver [%s]!", hosts.c_str()));
        _zk_handle = zookeeper_init(hosts.c_str(), global_watcher, timeout, 0, (void*)"zk init", 0);

        //判断zookeeper连接状态直到连接ok，超时则退出程序
        int conn_status_cnt = 0;
        while (_zk_handle == NULL || zoo_state(_zk_handle) != ZOO_CONNECTED_STATE)
        {
            usleep(100 * 1000);
            conn_status_cnt++;
            if (conn_status_cnt >= 50)
            {
                CS_ERROR(("zk not connected!"));
                exit(1);
            }
        }
        CS_INFO(("zk connect ok!"));

        //创建监视基路径znode
        struct Stat stat;
        int ret = zoo_exists(_zk_handle, MONITOR_BASE_PATH, 0, &stat);
        if (ret == ZNONODE)
        {
            CS_INFO(("zk monitor base path is not exists, code[%d]", ret));
            ret = zoo_create(_zk_handle, MONITOR_BASE_PATH, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
            CS_INFO(("zk monitor base path create, code[%d]", ret));
            if (ret != ZOK)
            {
                CS_ERROR(("zk monitor base path create failed!"));
                exit(1);
            }
        }
        else
        {
            CS_INFO(("zk monitor base path maybe exists, code[%d]", ret));
        }
    }

    void zk_uninit()
    {
        CS_INFO(("uninit zk...!"));
        if (_zk_handle != NULL)
        {
            zookeeper_close(_zk_handle);
        }
    }

    void is_need_backup(unsigned backup)
    {
        _notify_backup = backup;
    }

    void create_pid_paths(const std::set<std::string>& pids)
    {
        //创建监视每个pid路径znode
        std::set<std::string>::iterator iter = pids.begin();
        for (; iter != pids.end(); ++iter)
        {
            std::string pid = *iter;
            char path[128] = { 0 };
            snprintf(path, sizeof(path), "%s/%s", MONITOR_BASE_PATH, pid.c_str());

            struct Stat stat;
            int ret = zoo_exists(_zk_handle, path, 0, &stat);
            if (ret == ZNONODE)
            {
                CS_INFO(("zk monitor pid[%s] path is not exists, code[%d]", pid.c_str(), ret));
                ret = zoo_create(_zk_handle, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
                CS_INFO(("zk monitor pid[%s] path create, code[%d]", pid.c_str(), ret));
                if (ret != ZOK)
                {
                    CS_ERROR(("zk monitor pid[%s] path create failed!", pid.c_str()));
                }
            }
            else
            {
                CS_INFO(("zk monitor pid[%s] path maybe exists, code[%d]", pid.c_str(), ret));
            }
        }
    }

    static void inner_update(zhandle_t* zk_handle, const char* zk_node_path, const std::string& file_path, const int from_zk)
    {
        if (zk_handle == NULL)
        {
            CS_ERROR(("zk monitor handler is null!"));
            return;
        }

        CS_INFO(("zk monitor file path[%s], zk node path[%s]", file_path.c_str(), zk_node_path));
        struct Stat stat;
        int ret = zoo_wexists(zk_handle, zk_node_path, exists_watcher, NULL, &stat);
        if (ret == ZNONODE || ret == ZOK)
        {
            std::ifstream in(file_path.c_str());
            if (!in.is_open())
            {
                CS_ERROR(("zk monitor read file[%s] content error!", file_path.c_str()));
                return;
            }

            std::streampos pos = in.tellg();
            in.seekg(0, std::ios::end);
            uint32_t file_size = in.tellg();
            in.seekg(pos);

            std::streamsize read_file_len;
            char* file_buffer = new char[file_size + 1];
            memset(file_buffer, 0, file_size + 1);
            in.read(file_buffer, file_size);
            read_file_len = in.gcount();
            in.close();

            if (ret == ZNONODE)
            {
                //如果zookeeper中不存在相关path，则直接创建path，默认znode最大存储为1M，超过此大小会创建失败，返回的ret值可能是-4
                ret = zoo_create(zk_handle, zk_node_path, file_buffer, (int)read_file_len, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
                CS_INFO(("zk monitor file create, code[%d]", ret));
            }
            else if (ret == ZOK)
            {
                //判断文件内容是否与znode中的内容一致
                char zk_buffer[1050000] = { 0 };
                int zk_len = sizeof(zk_buffer);
                ret = zoo_get(zk_handle, zk_node_path, 0, zk_buffer, &zk_len, &stat);
                CS_INFO(("zk monitor znode buffer get, code[%d], zk len[%d]", ret, zk_len));
                if (ret == ZOK)
                {
                    int is_same = 1;
                    if (read_file_len != zk_len)
                    {
                        is_same = 0;
                    }
                    else
                    {
                        std::string file_buffer_md5 = get_buffer_md5(file_buffer, read_file_len);
                        std::string zk_buffer_md5 = get_buffer_md5(zk_buffer, zk_len);
                        CS_INFO(("zk monitor check file and znode buffer, file value[%s], znode value[%s]", file_buffer_md5.c_str(), zk_buffer_md5.c_str()));
                        if (file_buffer_md5 != zk_buffer_md5)
                        {
                            is_same = 0;
                        }
                    }

                    if (is_same == 0)
                    {
                        if (from_zk)
                        {
                            if (_notify_backup)
                            {
                                struct timeval tv;
                                gettimeofday(&tv, NULL);
                                time_t ltime = tv.tv_sec;
                                struct tm* t = localtime(&ltime);
                                char ch_stamp[100] = { 0 };
                                snprintf(ch_stamp, sizeof(ch_stamp), ".%04d%02d%02d%02d%02d%02d%06ld",
                                    t->tm_year + 1900,
                                    t->tm_mon + 1,
                                    t->tm_mday,
                                    t->tm_hour,
                                    t->tm_min,
                                    t->tm_sec,
                                    tv.tv_usec);
                                std::string stamp_str = ch_stamp;
                                std::string backup = "cp -rf \"" + file_path + "\" \"" + file_path + stamp_str  + "\"";
                                system(backup.c_str());
                            }
                            //如果是程序load或reload配置加载，则使用znode中的内容更新本地被监视文件;
                            //这样被监视的文件将与创建该path的第一个服务器上的文件同步一致
                            std::ofstream out(file_path.c_str());
                            out.write(zk_buffer, zk_len);
                            out.close();
                            CS_INFO(("zk monitor update file content from zk"));
                        }
                        else
                        {
                            //如果是在程序运行过程中，则将修改的本地文件更新到znode中的内容
                            ret = zoo_set(zk_handle, zk_node_path, file_buffer, (int)read_file_len, -1);
                            CS_INFO(("zk monitor file content changed, code[%d]", ret));
                        }
                    }
                }
            }

            delete[] file_buffer;
            file_buffer = NULL;
        }
        else
        {
            CS_ERROR(("zk monitor read file exist error, code[%d]", ret));
        }
    }

    static void exists_watcher(zhandle_t* zh, int type, int state, const char* zk_node_path, void* watcher_ctx)
    {
        UNUSED(watcher_ctx);
        std::string file = zk_node_path;
        size_t pos = file.find_last_of("/");
        if (pos != std::string::npos)
        {
            file = file.substr(pos + 1);
            file = cs_util::replace_all(file, ":", "/");
        }

        CS_INFO(("zk exists watcher type[%d], state[%d], node path[%s], file[%s]", type, state, zk_node_path, file.c_str()));
        if (state == ZOO_CONNECTED_STATE && type == ZOO_CHANGED_EVENT)
        {
            inner_update(zh, zk_node_path, file, 1);
        }
        else
        {
            struct Stat stat;
            int ret = zoo_wexists(zh, zk_node_path, exists_watcher, NULL, &stat);
            CS_INFO(("zk re exists, code[%d]", ret));
        }
    }

    void update_monitor_path(const std::string& pid, const std::string& file_path, const int from_zk)
    {
        std::string path_name = cs_util::replace_all(file_path, "/", ":");
        char zk_node_path[2600] = { 0 };
        snprintf(zk_node_path, sizeof(zk_node_path), "%s/%s/%s", MONITOR_BASE_PATH, pid.c_str(), path_name.c_str());
        inner_update(_zk_handle, zk_node_path, file_path, from_zk);
    }

    void delete_monitor_path(const std::string& pid, const std::string& file_path)
    {
        if (_zk_handle != NULL)
        {
            std::string path_name = cs_util::replace_all(file_path, "/", ":");
            char zk_node_path[2600] = { 0 };
            snprintf(zk_node_path, sizeof(zk_node_path), "%s/%s/%s", MONITOR_BASE_PATH, pid.c_str(), path_name.c_str());
            CS_INFO(("zk monitor delete node path[%s]", zk_node_path));
            int ret = zoo_delete(_zk_handle, zk_node_path, -1);
            CS_INFO(("zk monitor delete result, code[%d]", ret));
        }
    }
}

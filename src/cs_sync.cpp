#include <errno.h>
#include <unistd.h>
#include <map>
#include <set>
#include <sys/inotify.h>
#include "cs_util.h"
#include "cs_log.h"
#include "cs_md5.h"
#include "cs_zk.h"
#include "cs_epoll.h"
#include "cs_sync.h"

#define INOTIFY_BUF_LEN 10240

namespace cs_sync
{
    struct cs_context
    {
        int _ev_size;
        int _ev_timeout;
        int _inotify_fd;
        std::map<std::string, std::pair<std::string, int> > _notify_map;
        volatile unsigned _notify_backup;
    };

    //注意：vi对被监控的文件进行编辑，并不会触发IN_MODIFY事件，
    //而是会触发IN_DELETE_SELF、 IN_MOVE_SELF和IN_IGNORED三个事件
    static const uint32_t _i_flag = IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF;
    static struct cs_context* _cs_ctx = NULL;

    void sync_start(struct cs_conf_info* conf)
    {
        CS_INFO(("init sync...!"));
        if (conf == NULL)
        {
            CS_ERROR(("conf is null!"));
            exit(1);
        }

        cs_zk::zk_init(conf->_zk_hosts, conf->_zk_timeout);

        int fd = inotify_init();
        if (fd < 0)
        {
            int err = errno;
            CS_ERROR(("inotify init is failed: [%d:%s]!", err, strerror(err)));
            exit(1);
        }

        _cs_ctx = new struct cs_context();
        if (_cs_ctx == NULL)
        {
            CS_ERROR(("new context object failed!"));
            int status = close(fd);
            if (status < 0)
            {
                int err = errno;
                CS_ERROR(("close inotify fd failed: [%d:%s]!", err, strerror(err)));
            }
            exit(1);
        }

        _cs_ctx->_ev_size = conf->_ev_size;
        _cs_ctx->_ev_timeout = conf->_ev_timeout;
        _cs_ctx->_inotify_fd = fd;

        cs_util::set_non_blocking(_cs_ctx->_inotify_fd);
        sync_reload(conf->_monitor_infos, conf->_need_backup);
        cs_epoll::event_base_init(_cs_ctx->_ev_size, &sync_cb);
        cs_epoll::event_add(_cs_ctx->_inotify_fd);
    }

    void sync_stop()
    {
        if (_cs_ctx == NULL || _cs_ctx->_inotify_fd < 0)
        {
            return;
        }

        cs_epoll::event_del(_cs_ctx->_inotify_fd);
        cs_epoll::event_base_uninit();
        sync_release();

        if (_cs_ctx->_inotify_fd > 0)
        {
            int status = close(_cs_ctx->_inotify_fd);
            if (status < 0)
            {
                int err = errno;
                CS_ERROR(("close inotify fd failed: [%d:%s]!", err, strerror(err)));
            }
            _cs_ctx->_inotify_fd = -1;
        }

        delete _cs_ctx;
        _cs_ctx = NULL;

        cs_zk::zk_uninit();
        CS_INFO(("uninit sync...!"));
    }

    static void sync_reload(const std::string& pid, const std::set<std::string>& files, std::set<std::string>& tmp_set)
    {
        if (_cs_ctx == NULL)
        {
            return;
        }

        std::set<std::string>::iterator file_iter = files.begin();
        for (; file_iter != files.end(); ++file_iter)
        {
            std::string file = *file_iter;
            std::set<std::string>::iterator iter = tmp_set.find(file);
            if (iter != tmp_set.end())
            {
                //从临时set中移除仍在监控的文件，则剩余在临时set中的为不再继续监控的文件
                tmp_set.erase(iter);
                CS_INFO(("file still in monitor, file[%s]", file.c_str()));
            }
            else
            {
                //添加监控
                int wd = inotify_add_watch(_cs_ctx->_inotify_fd, file.c_str(), _i_flag);
                if (wd < 0)
                {
                    int err = errno;
                    CS_ERROR(("inotify add watch[%d] is failed: [%d:%s]!", _cs_ctx->_inotify_fd, err, strerror(err)));
                }
                else
                {
                    std::pair<std::string, int> pid_wd = make_pair(pid, wd);
                    _cs_ctx->_notify_map[file] = pid_wd;
                }
                CS_INFO(("file maybe new in monitor, file[%s], pid[%s], wd[%d]", file.c_str(), pid.c_str(), wd));
            }

            //更新zk路径内容
            cs_zk::update_monitor_path(pid, file, 1);
        }
    }

    void sync_reload(std::vector<struct cs_monitor_info>& infos, const unsigned backup)
    {
        if (_cs_ctx == NULL)
        {
            return;
        }

        std::set<std::string> tmp_set;
        std::map<std::string, std::pair<std::string, int> >::iterator notify_iter = _cs_ctx->_notify_map.begin();
        for (; notify_iter != _cs_ctx->_notify_map.end(); ++notify_iter)
        {
            //获取所有路径保存到临时set中
            tmp_set.insert(notify_iter->first);
        }

        std::set<std::string> pids;
        std::vector<struct cs_monitor_info>::iterator iter = infos.begin();
        for (; iter != infos.end(); ++iter)
        {
            pids.insert(iter->_monitor_pid);
            sync_reload(iter->_monitor_pid, iter->_monitor_files, tmp_set);
        }

        //创建所有pid的zk路径（路径不存在的话）
        cs_zk::create_pid_paths(pids);

        //遍历临时set
        std::set<std::string>::iterator tmp_iter = tmp_set.begin();
        for (; tmp_iter != tmp_set.end();)
        {
            if (tmp_iter != NULL)
            {
                std::map<std::string, std::pair<std::string, int> >::iterator iter = _cs_ctx->_notify_map.find((*tmp_iter));
                if (iter != _cs_ctx->_notify_map.end())
                {
                    //删除监控
                    inotify_rm_watch(_cs_ctx->_inotify_fd, iter->second.second);
                    //删除本地不再监控的zk路径
                    cs_zk::delete_monitor_path(iter->second.first, *tmp_iter);
                    _cs_ctx->_notify_map.erase(iter);
                }
                CS_INFO(("file maybe deleted out of monitor, file[%s]", (*tmp_iter).c_str()));
            }
            tmp_set.erase(tmp_iter++);
        }

        _cs_ctx->_notify_backup = backup;
        cs_zk::is_need_backup(backup);
    }

    void sync_release()
    {
        if (_cs_ctx == NULL)
        {
            return;
        }

        std::map<std::string, std::pair<std::string, int> >::iterator notify_iter = _cs_ctx->_notify_map.begin();
        for (; notify_iter != _cs_ctx->_notify_map.end();)
        {
            //删除监控
            inotify_rm_watch(_cs_ctx->_inotify_fd, notify_iter->second.second);
            //删除本地不再监控的zk路径
            cs_zk::delete_monitor_path(notify_iter->second.first, notify_iter->first);
            _cs_ctx->_notify_map.erase(notify_iter++);
        }
    }

    static void update_wd(int old_wd)
    {
        if (_cs_ctx == NULL)
        {
            return;
        }

        std::map<std::string, std::pair<std::string, int> >::iterator notify_iter = _cs_ctx->_notify_map.begin();
        for (; notify_iter != _cs_ctx->_notify_map.end(); ++notify_iter)
        {
            std::string file = notify_iter->first;
            std::string pid = notify_iter->second.first;
            if (notify_iter->second.second == old_wd)
            {
                inotify_rm_watch(_cs_ctx->_inotify_fd, old_wd);
                int wd = inotify_add_watch(_cs_ctx->_inotify_fd, file.c_str(), _i_flag);
                if (wd < 0)
                {
                    int err = errno;
                    CS_ERROR(("inotify add watch[%d] is failed: [%d:%s]!", _cs_ctx->_inotify_fd, err, strerror(err)));
                }
                else
                {
                    std::pair<std::string, int> pid_wd = make_pair(pid, wd);
                    _cs_ctx->_notify_map[file] = pid_wd;
                    CS_INFO(("inotify file[%s] watch from old[%d] to new[%d]!", file.c_str(), old_wd, wd));
                }
                break;
            }
        }
    }

    static void update_file_content(int wd)
    {
        if (_cs_ctx == NULL)
        {
            return;
        }

        std::map<std::string, std::pair<std::string, int> >::iterator notify_iter = _cs_ctx->_notify_map.begin();
        for (; notify_iter != _cs_ctx->_notify_map.end(); ++notify_iter)
        {
            std::string file = notify_iter->first;
            std::string pid = notify_iter->second.first;
            if (notify_iter->second.second == wd)
            {
                //更新文件内容
                CS_INFO(("inotify pid[%s] file[%s] content changed!", pid.c_str(), file.c_str()));
                //更新zk路径内容
                cs_zk::update_monitor_path(pid, file, 0);
            }
        }
    }

    int sync_cb(int fd, uint32_t events)
    {
        if (_cs_ctx == NULL)
        {
            return (-1);
        }

        if (_cs_ctx->_inotify_fd != fd)
        {
            CS_WARN(("fd[%d] is not inotify fd[%d].", _cs_ctx->_inotify_fd, fd));
            return (-1);
        }

        CS_INFO(("fd[%d] events[%u]", fd, events));

        if (!(events & CS_EVENT_READ))
        {
            return (-1);
        }

        std::map<int, uint16_t> wd_mask;
        char buf[INOTIFY_BUF_LEN] = { 0 };
        int read_len = 0;
        while ((read_len = read(fd, buf, INOTIFY_BUF_LEN)) > 0)
        {
            int iev_offset = 0;
            while (read_len > 0)
            {
                struct inotify_event* iev = (struct inotify_event*)&buf[iev_offset];
                if (iev == NULL)
                {
                    CS_INFO(("inotify event is null"));
                    break;
                }
                if (iev->len > 0)
                {
                    CS_INFO(("monitor file[%s]", iev->name));
                }

                int iev_len = sizeof(struct inotify_event) + iev->len;
                iev_offset += iev_len;
                read_len -= iev_len;

                std::map<int, uint16_t>::iterator iter = wd_mask.find(iev->wd);
                if (iev->mask != IN_IGNORED)
                {
                    //被监视的文件内容被修改了
                    if (iter == wd_mask.end())
                    {
                        wd_mask[iev->wd] = 10;
                    }
                    else
                    {
                        wd_mask[iev->wd] = 10 + (wd_mask[iev->wd] % 10);
                    }
                }
                else
                {
                    //被监视的文件被自动忽略掉了
                    if (iter == wd_mask.end())
                    {
                        wd_mask[iev->wd] = 1;
                    }
                    else
                    {
                        wd_mask[iev->wd] = (wd_mask[iev->wd] / 10) * 10 + 1;
                    }
                }

                CS_INFO(("monitor file[%d, %u, %u, %u, %d, %d, %d]", iev->wd, iev->mask, iev->cookie, iev->len, iev_len, iev_offset, read_len));
            }
        }

        std::map<int, uint16_t>::iterator iter = wd_mask.begin();
        for (; iter != wd_mask.end(); ++iter)
        {
            int flag = iter->second;
            if ((flag % 10) > 0)
            {
                //被监视的文件被自动忽略掉了，重新添加监视
                update_wd(iter->first);
            }
            if ((flag / 10) > 0)
            {
                //被监视的文件内容被修改了
                update_file_content(iter->first);
            }
        }

        CS_INFO(("events read over!"));
        return 0;
    }

    int sync_loop()
    {
        if (_cs_ctx == NULL)
        {
            return (-1);
        }

        int fds = cs_epoll::event_wait(_cs_ctx->_ev_timeout);
        if (fds < 0)
        {
            return fds;
        }

        return 0;
    }
}

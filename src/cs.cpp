#include <sys/stat.h>
#include <iostream>
#include <set>
#include "cs_util.h"
#include "cs_option.h"
#include "cs_signal.h"
#include "cs_log.h"
#include "cs_daemon.h"
#include "cs_sync.h"

static const char CS_CONF_FILE_NAME[] = "comsync.yml";
static char _conf_file[2600] = { 0 };
static int _quit_flag = 0;
int _daemonize = 0;

void reload_part_conf()
{
    struct cs_conf_info conf;
    if (cs_conf::conf_load(&conf, _conf_file) == 0)
    {
        cs_sync::sync_reload(conf._monitor_infos, conf._need_backup);
    }
    else
    {
        CS_ERROR(("reload configure file failed, maybe configure the same monitor files!"));
    }
}

void reopen_log()
{
    cs_log::log_reopen();
}

void quit_graceful(int flag)
{
    _quit_flag = flag;
}

static int get_conf_path(const char* work_path)
{
    snprintf(_conf_file, sizeof(_conf_file), "%s/../conf/%s", work_path, CS_CONF_FILE_NAME);
    if (access(_conf_file, F_OK) != 0)
    {
        std::cout << "conf path[" << _conf_file << "] does not exist!" << std::endl;
        return (-1);
    }
    std::cout << "conf path[" << _conf_file << "]" << std::endl;

    return 0;
}

static std::string get_log_path(const char* work_path)
{
    char log_path[2600] = { 0 };
    snprintf(log_path, sizeof(log_path), "%s/../log", work_path);
    if (access(log_path, F_OK) != 0)
    {
        std::cout << "log path[" << log_path << "] does not exist!" << std::endl;
        int status = mkdir(log_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        if (status < 0)
        {
            std::cout << "create log path[" << log_path << "] failed!" << std::endl;
            return "";
        }
        else
        {
            std::cout << "create log path[" << log_path << "] success!" << std::endl;
        }
    }
    std::cout << "log path[" << log_path << "]" << std::endl;

    std::string log_path_str = log_path;
    return log_path_str;
}

int main(int argc, char** argv)
{
    if (cs_option::options_check(argc, argv) != 0)
    {
        exit(1);
    }

    cs_daemon::daemon_run(_daemonize, 0);
    cs_signal::signal_init();
    cs_signal::set_sig_hup_cb(reload_part_conf);
    cs_signal::set_sig_usr_cb(reopen_log);
    cs_signal::set_sig_quit_cb(quit_graceful);

    //获取服务程序当前路径
    std::string work_path = cs_util::get_work_path();
    if (work_path.empty())
    {
        return (-1);
    }

    //获取配置文件
    if (get_conf_path(work_path.c_str()) != 0)
    {
        return (-1);
    }

    //获取日志路径
    std::string log_path = get_log_path(work_path.c_str());
    if (log_path.empty())
    {
        return (-1);
    }

    //加载配置文件
    struct cs_conf_info conf;
    if (cs_conf::conf_load(&conf, _conf_file) != 0)
    {
        CS_ERROR(("load configure file failed!"));
        cs_signal::signal_uninit();
        return (-1);
    }

    //初始化日志
    char log_file[2600] = { 0 };
    snprintf(log_file, sizeof(log_file), "%s/%s", log_path.c_str(), conf._log_file.c_str());
    cs_log::log_init(conf._log_level, log_file);

    cs_sync::sync_start(&conf);
    while (1)
    {
        int status = cs_sync::sync_loop();
        if (status < 0)
        {
            break;
        }

        if (_quit_flag)
        {
            CS_INFO(("quit sync...!"));
            break;
        }
    }
    cs_sync::sync_stop();

    cs_log::log_uninit();
    cs_signal::signal_uninit();

    return 0;
}

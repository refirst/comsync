#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include "cs_daemon.h"

namespace cs_daemon
{
    void daemon_run(int daemon, int cur_work)
    {
        if (daemon == 0)
        {
            return;
        }

        //创建第一个子进程
        pid_t pid = fork();
        if (pid < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "first fork failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return;
        }
        else if (pid > 0)
        {
            //退出父进程
            exit(0);
        }

        //在第一个子进程中创建新的会话，并担任会话组leader
        pid_t sid = setsid();
        if (sid < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "setsid failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return;
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "sighup failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return;
        }

        //创建第二个子进程
        pid = fork();
        if (pid < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "second fork failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return;
        }
        else if (pid > 0)
        {
            //退出第一个子进程
            exit(0);
        }

        if (cur_work != 0)
        {
            //设置当前工作路径为根路径
            int status = chdir("/");
            if (status < 0)
            {
                int err = errno;
                char buf[256] = { 0 };
                sprintf(buf, "chdir failed: [%d:%s]!", err, strerror(err));
                std::cout << buf << std::endl;
                return;
            }
        }

        //设置文件权限掩码
        umask(0);

        //将标准输入、输出、错误重定向到/dev/null，也可直接关闭标准输入、输出、错误文件描述符
        int fd = open("/dev/null", O_RDWR);
        if (fd < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "open dev null failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return;
        }

        int status = dup2(fd, STDIN_FILENO);
        if (status < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "dup2 stdin failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            close(fd);
            return;
        }

        status = dup2(fd, STDOUT_FILENO);
        if (status < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "dup2 stdout failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            close(fd);
            return;
        }

        status = dup2(fd, STDERR_FILENO);
        if (status < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "dup2 stderr failed: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            close(fd);
            return;
        }

        if (fd > STDERR_FILENO)
        {
            status = close(fd);
            if (status < 0)
            {
                int err = errno;
                char buf[256] = { 0 };
                sprintf(buf, "close fd failed: [%d:%s]!", err, strerror(err));
                std::cout << buf << std::endl;
                return;
            }
        }

        std::cout << "run as a daemon!" << std::endl;
    }
}

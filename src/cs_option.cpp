#include <sys/utsname.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <errno.h>
#include "cs_option.h"

static const char CS_VERSION[] = "1.0.0.1";
static int _show_help = 0;
static int _show_version = 0;
extern int _daemonize;

static const struct option _long_opts[] = {
    {"help",      no_argument, NULL, 'h'},
    {"version",   no_argument, NULL, 'v'},
    {"daemonize", no_argument, NULL, 'd'},
    {NULL,        0,           NULL,  0 }
};
static const char _short_opts[] = "hvd";

namespace cs_option
{
    static int get_options(int argc, char** argv)
    {
        while (1)
        {
            int ch = getopt_long(argc, argv, _short_opts, _long_opts, NULL);
            if (ch == -1)
            {
                break;
            }

            switch (ch)
            {
                case 'h':
                    _show_help = 1;
                    break;
                case 'v':
                    _show_version = 1;
                    break;
                case 'd':
                    _daemonize = 1;
                    break;
                default:
                    return (-1);
            }
        }

        return 0;
    }

    int options_check(int argc, char** argv)
    {
        int opt = get_options(argc, argv);
        if (opt != 0 || _show_help)
        {
            std::cout << "Usage: comsync [-hvd]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --help      : show help" << std::endl;
            std::cout << "  -v, --version   : show version information" << std::endl;
            std::cout << "  -d, --daemonize : run as a daemon" << std::endl;
            return (-1);
        }

        if (_show_version)
        {
            const char req_sys_ver[] = ", must run in Linux-2.6.13 or later OS version.";
            struct utsname un;
            int status = uname(&un);
            if (status < 0)
            {
                std::cout << "comsync-" << CS_VERSION
                    << req_sys_ver
                    << std::endl;
            }
            else
            {
                std::cout << "comsync-" << CS_VERSION
                    << req_sys_ver
                    << " Now OS is " << un.sysname
                    << "-" << un.release
                    << std::endl;
            }
            return (-1);
        }

        return 0;
    }
}

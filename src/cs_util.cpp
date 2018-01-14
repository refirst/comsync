#include <iostream>
#include <fcntl.h>
#include "cs_util.h"

namespace cs_util
{
    static const std::string _space_str = "\t\r\n \f\v";

    std::string get_work_path()
    {
        char ch_work_path[2600] = { 0 };
        if (readlink("/proc/self/exe", ch_work_path, 2600) < 0)
        {
            std::cout << "get exe directory error!!!" << std::endl;
            return "";
        }
        char* end_ptr = strrchr(ch_work_path, '/');
        if (end_ptr == NULL)
        {
            std::cout << "get exe parent directory error!!!" << std::endl;
            return "";
        }
        *end_ptr = '\0';

        std::string work_path = ch_work_path;
        return work_path;
    }

    int set_non_blocking(int fd)
    {
        if (fd < 0)
        {
            return (-1);
        }

        int fd_old = fcntl(fd, F_GETFL, 0);
        if (fd_old < 0)
        {
            return (-1);
        }

        return fcntl(fd, F_SETFL, fd_old | O_NONBLOCK);
    }

    std::string trim_space_left(const std::string& src)
    {
        std::string ret = src;
        ret.erase(0, ret.find_first_not_of(_space_str));
        return ret;
    }

    std::string trim_space_right(const std::string& src)
    {
        std::string ret = src;
        ret.erase(ret.find_last_not_of(_space_str) + 1);
        return ret;
    }

    std::string trim_space(const std::string& src)
    {
        std::string ret = src;
        ret.erase(0, ret.find_first_not_of(_space_str));
        ret.erase(ret.find_last_not_of(_space_str) + 1);
        return ret;
    }

    std::string replace_all(const std::string& src, const std::string& old_s, const std::string& new_s)
    {
        std::string ret = src;
        size_t pos = ret.find_first_of(old_s);
        while (pos != std::string::npos)
        {
            ret.replace(pos, 1, new_s);
            pos = ret.find_first_of(old_s);
        }
        return ret;
    }
}

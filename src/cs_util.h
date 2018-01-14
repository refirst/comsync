#ifndef CS_UTIL_H
#define CS_UTIL_H

#include <string>

#define UNUSED(x) ((void)(x))

namespace cs_util
{
    std::string get_work_path();
    int set_non_blocking(int fd);
    std::string trim_space_left(const std::string& src);
    std::string trim_space_right(const std::string& src);
    std::string trim_space(const std::string& src);
    std::string replace_all(const std::string& src, const std::string& old_s, const std::string& new_s);
}

#endif

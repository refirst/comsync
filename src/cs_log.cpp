#include <string>
#include <fstream>
#include "cs_log.h"

namespace cs_log
{
    static std::string _log_file = "";
    static FILE* _log_file_handler = NULL;

    static FILE* inner_open_log(const char* file, int is_re)
    {
        FILE* handler = fopen(file, "a");
        std::string flag = "set";
        if (is_re)
        {
            flag = "reopen";
        }

        if (handler != NULL)
        {
            zoo_set_log_stream(handler);
            CS_INFO(("log file [%s] is [%s]", file, flag.c_str()));
        }
        else
        {
            CS_WARN(("log file [%s] is [%s] error", file, flag.c_str()));
        }

        return handler;
    }

    void log_init(int level, const char* file)
    {
        ZooLogLevel log_level = ZOO_LOG_LEVEL_INFO;
        switch (level)
        {
            case 1:
                log_level = ZOO_LOG_LEVEL_ERROR;
                break;
            case 2:
                log_level = ZOO_LOG_LEVEL_WARN;
                break;
            case 3:
                log_level = ZOO_LOG_LEVEL_INFO;
                break;
            case 4:
                log_level = ZOO_LOG_LEVEL_DEBUG;
                break;
        }
        zoo_set_debug_level(log_level);

        _log_file = file;
        if (_log_file_handler == NULL)
        {
            _log_file_handler = inner_open_log(file, 0);
        }
        CS_INFO(("set log level[%d]", log_level));
    }

    void log_uninit()
    {
        if (_log_file_handler != NULL)
        {
            fflush(_log_file_handler);
            fclose(_log_file_handler);
            _log_file_handler = NULL;
        }
    }

    void log_reopen()
    {
        if (access(_log_file.c_str(), F_OK) != 0)
        {
            CS_WARN(("log file [%s] not exists", _log_file.c_str()));
            return;
        }

        FILE* handler = inner_open_log(_log_file.c_str(), 1);
        log_uninit();
        _log_file_handler = handler;
    }
}

#include <errno.h>
#include <iostream>
#include <string>
#include <vector>
#include "yaml.h"
#include "cs_conf.h"
#include "cs_log.h"
#include "cs_util.h"

namespace cs_conf
{
    struct conf_kv
    {
        int _level;
        std::string _k;
        std::vector<std::string> _v;
    };

    static void inner_scan_token(yaml_parser_t& parser, std::vector<struct conf_kv>& cf_kvs)
    {
        int level = 0;
        int kv_mask = 1;
        yaml_token_t token;
        do
        {
            yaml_parser_scan(&parser, &token);
            switch (token.type)
            {
                case YAML_NO_TOKEN:
                    std::cout << "yaml no token..." << std::endl;
                    break;
                case YAML_STREAM_START_TOKEN:
                    std::cout << "yaml stream start token..." << std::endl;
                    break;
                case YAML_STREAM_END_TOKEN:
                    std::cout << "yaml stream end token..." << std::endl;
                    break;
                case YAML_VERSION_DIRECTIVE_TOKEN:
                    std::cout << "yaml version directive token..." << std::endl;
                    break;
                case YAML_TAG_DIRECTIVE_TOKEN:
                    std::cout << "yaml tag directive token..." << std::endl;
                    break;
                case YAML_DOCUMENT_START_TOKEN:
                    std::cout << "yaml document start token..." << std::endl;
                    break;
                case YAML_DOCUMENT_END_TOKEN:
                    std::cout << "yaml document end token..." << std::endl;
                    break;
                case YAML_BLOCK_SEQUENCE_START_TOKEN:
                    {
                        std::cout << "yaml block sequence start token..." << std::endl;
                        level++;
                    }
                    break;
                case YAML_BLOCK_MAPPING_START_TOKEN:
                    {
                        std::cout << "yaml block mapping start token..." << std::endl;
                        level++;
                    }
                    break;
                case YAML_BLOCK_END_TOKEN:
                    {
                        std::cout << "yaml block end token..." << std::endl;
                        level--;
                    }
                    break;
                case YAML_FLOW_SEQUENCE_START_TOKEN:
                    std::cout << "yaml folw sequence start token..." << std::endl;
                    break;
                case YAML_FLOW_SEQUENCE_END_TOKEN:
                    std::cout << "yaml flow sequence end token..." << std::endl;
                    break;
                case YAML_FLOW_MAPPING_START_TOKEN:
                    std::cout << "yaml flow mapping start token..." << std::endl;
                    break;
                case YAML_FLOW_MAPPING_END_TOKEN:
                    std::cout << "yaml flow mapping end token..." << std::endl;
                    break;
                case YAML_BLOCK_ENTRY_TOKEN:
                    std::cout << "yaml block entry token..." << std::endl;
                    break;
                case YAML_FLOW_ENTRY_TOKEN:
                    std::cout << "yaml flow entry token..." << std::endl;
                    break;
                case YAML_KEY_TOKEN:
                    {
                        std::cout << "yaml key token..." << std::endl;
                        struct conf_kv cf_kv;
                        cf_kvs.push_back(cf_kv);
                        kv_mask = 1;
                    }
                    break;
                case YAML_VALUE_TOKEN:
                    {
                        std::cout << "yaml vlaue token..." << std::endl;
                        kv_mask = 0;
                    }
                    break;
                case YAML_ALIAS_TOKEN:
                    std::cout << "yaml alias token..." << std::endl;
                    break;
                case YAML_ANCHOR_TOKEN:
                    std::cout << "yaml anchor token..." << std::endl;
                    break;
                case YAML_TAG_TOKEN:
                    std::cout << "yaml tag token..." << std::endl;
                    break;
                case YAML_SCALAR_TOKEN:
                    {
                        std::string tmp;
                        tmp.assign((char*)token.data.scalar.value, token.data.scalar.length);
                        tmp = cs_util::trim_space(tmp);
                        std::cout << "yaml scalar token..." << tmp.c_str() << "===" << kv_mask << "===" << level << std::endl;
                        int idx = cf_kvs.size() - 1;
                        if (idx < 0)
                        {
                            std::cout << "error index is less than zero!" << tmp.c_str() << std::endl;
                            break;
                        }

                        cf_kvs[idx]._level = level;
                        if (kv_mask)
                        {
                            cf_kvs[idx]._k = tmp;
                        }
                        else
                        {
                            cf_kvs[idx]._v.push_back(tmp);
                        }
                    }
                    break;
                default:
                    std::cout << "other token..." << std::endl;
                    break;
            }
            if (token.type != YAML_STREAM_END_TOKEN)
            {
                yaml_token_delete(&token);
            }
        } while(token.type != YAML_STREAM_END_TOKEN);
        yaml_token_delete(&token);
    }

    static int inner_get_conf(std::vector<struct conf_kv>& cf_kvs, struct cs_conf_info* conf)
    {
        std::set<std::string> check_set;
        std::vector<struct conf_kv>::iterator kv_iter = cf_kvs.begin();
        for (; kv_iter != cf_kvs.end(); ++kv_iter)
        {
            std::cout << kv_iter->_k << std::endl;
            if (kv_iter->_k == "log_level")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    conf->_log_level = 3;
                }
                else
                {
                    conf->_log_level = atoi(kv_iter->_v[0].c_str());
                }
            }
            else if (kv_iter->_k == "log_file")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    conf->_log_file = "comsync.log";
                }
                else
                {
                    conf->_log_file = kv_iter->_v[0];
                }
            }
            else if (kv_iter->_k == "need_backup")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    conf->_need_backup = 1;
                }
                else
                {
                    conf->_need_backup = atoi(kv_iter->_v[0].c_str());
                }
            }
            else if (kv_iter->_k == "event_size")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    conf->_ev_size = 16;
                }
                else
                {
                    conf->_ev_size = atoi(kv_iter->_v[0].c_str());
                }
            }
            else if (kv_iter->_k == "event_timeout")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    conf->_ev_timeout = 1;
                }
                else
                {
                    conf->_ev_timeout = atoi(kv_iter->_v[0].c_str());
                }
            }
            else if (kv_iter->_k == "zk_timeout")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    conf->_zk_timeout = 60000;
                }
                else
                {
                    conf->_zk_timeout = atoi(kv_iter->_v[0].c_str());
                }
            }
            else if (kv_iter->_k == "zk_hosts")
            {
                if (kv_iter->_v.size() <= 0)
                {
                    return (-1);
                }
                else
                {
                    conf->_zk_hosts = "";
                    std::vector<std::string>::iterator iter = kv_iter->_v.begin();
                    for (; iter != kv_iter->_v.end(); ++iter)
                    {
                        if (conf->_zk_hosts.empty())
                        {
                            conf->_zk_hosts = (*iter);
                        }
                        else
                        {
                            conf->_zk_hosts = conf->_zk_hosts + "," + (*iter);
                        }
                    }
                }
            }
            else if (kv_iter->_k == "monitor_files")
            {
                int level = kv_iter->_level;
                int is_end = 0;
                while (1)
                {
                    kv_iter++;
                    if (kv_iter == cf_kvs.end())
                    {
                        is_end = 1;
                        break;
                    }
                    if (kv_iter->_level <= level)
                    {
                        break;
                    }
                    struct cs_monitor_info info;
                    info._monitor_pid = kv_iter->_k;
                    std::cout << "======" << info._monitor_pid << std::endl;
                    std::vector<std::string>::iterator iter = kv_iter->_v.begin();
                    for (; iter != kv_iter->_v.end(); ++iter)
                    {
                        std::set<std::string>::iterator check_iter = check_set.find(*iter);
                        if (check_iter != check_set.end())
                        {
                            //所有产品线的被监控的文件不能出现路径和文件名完全一样的文件
                            std::cout << "exist same monitor file!"  << std::endl;
                            return (-1);
                        }
                        check_set.insert(*iter);
                        info._monitor_files.insert(*iter);
                    }
                    conf->_monitor_infos.push_back(info);
                }
                if (is_end)
                {
                    break;
                }
            }
        }

        return 0;
    }

    int conf_load(struct cs_conf_info* conf, const char* path)
    {
        if (conf == NULL)
        {
            std::cout << "conf pointer is null." << std::endl;
            return (-1);
        }

        FILE* fi = fopen(path, "r");
        if (fi == NULL)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "failed to open configure file: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return (-1);
        }

        int ret = fseek(fi, 0L, SEEK_SET);
        if (ret < 0)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "failed to seek to the beginning of configure file: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return (-1);
        }

        yaml_parser_t parser;
        ret = yaml_parser_initialize(&parser);
        if (!ret)
        {
            int err = errno;
            char buf[256] = { 0 };
            sprintf(buf, "failed to initialize yaml parser: [%d:%s]!", err, strerror(err));
            std::cout << buf << std::endl;
            return (-1);
        }

        yaml_parser_set_input_file(&parser, fi);

        std::vector<struct conf_kv> cf_kvs;
        inner_scan_token(parser, cf_kvs);

        yaml_parser_delete(&parser);
        fclose(fi);

        return inner_get_conf(cf_kvs, conf);
    }
}

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "cs_log.h"
#include "cs_epoll.h"

namespace cs_epoll
{
    struct event_base
    {
        int _ev_fd;
        struct epoll_event* _evs;
        int _ev_size;
        cs_event_cb _ev_cb;
    };

    static struct event_base* _ev_base = NULL;

    void event_base_init(int size, cs_event_cb cb)
    {
        CS_INFO(("init event...!"));
        if (size <= 0)
        {
            CS_ERROR(("epoll size [%d] is less then zero!", size));
            exit(1);
        }

        int fd = epoll_create(size);
        if (fd < 0)
        {
            int err = errno;
            CS_ERROR(("epoll create size [%d] is failed: [%d:%s]!", size, err, strerror(err)));
            exit(1);
        }

        struct epoll_event* evs = new struct epoll_event[size];
        if (evs == NULL)
        {
            int err = errno;
            CS_ERROR(("new epoll event object failed: [%d:%s]!", err, strerror(err)));
            int status = close(fd);
            if (status < 0)
            {
                err = errno;
                CS_ERROR(("close epoll fd failed: [%d:%s]!", err, strerror(err)));
            }
            exit(1);
        }

        _ev_base = new struct event_base();
        if (_ev_base == NULL)
        {
            int err = errno;
            CS_ERROR(("new event base object failed: [%d:%s]!", err, strerror(err)));
            int status = close(fd);
            if (status < 0)
            {
                err = errno;
                CS_ERROR(("close epoll fd failed: [%d:%s]!", err, strerror(err)));
            }
            exit(1);
        }

        _ev_base->_ev_fd = fd;
        _ev_base->_evs = evs;
        _ev_base->_ev_size = size;
        _ev_base->_ev_cb = cb;
    }

    void event_base_uninit()
    {
        CS_INFO(("uninit event...!"));
        if (_ev_base == NULL)
        {
            return;
        }

        if (_ev_base->_evs != NULL)
        {
            delete[] _ev_base->_evs;
        }

        if (_ev_base->_ev_fd > 0)
        {
            int status = close(_ev_base->_ev_fd);
            if (status < 0)
            {
                int err = errno;
                CS_ERROR(("close epoll fd failed: [%d:%s]!", err, strerror(err)));
            }
            _ev_base->_ev_fd = -1;
        }

        delete _ev_base;
        _ev_base = NULL;
    }

    int event_add(int fd)
    {
        if (_ev_base == NULL)
        {
            return (-1);
        }

        if (fd < 0)
        {
            CS_ERROR(("fd is less than zero: [%d]!", fd));
            return (-2);
        }

        struct epoll_event ev;
        ev.events = (uint32_t)(EPOLLIN | EPOLLOUT);
        ev.data.fd = fd;

        int status = epoll_ctl(_ev_base->_ev_fd, EPOLL_CTL_ADD, fd, &ev);
        if (status < 0)
        {
            int err = errno;
            CS_ERROR(("epoll ctl add on epoll[%d] fd[%d] failed: [%d:%s]!", _ev_base->_ev_fd, fd, err, strerror(err)));
        }
        else
        {
            CS_INFO(("epoll ctl add on epoll[%d] fd[%d] success", _ev_base->_ev_fd, fd));
        }

        return status;
    }

    int event_del(int fd)
    {
        if (_ev_base == NULL)
        {
            return (-1);
        }

        if (fd < 0)
        {
            CS_ERROR(("fd is less than zero: [%d]!", fd));
            return (-2);
        }

        int status = epoll_ctl(_ev_base->_ev_fd, EPOLL_CTL_DEL, fd, NULL);
        if (status < 0)
        {
            int err = errno;
            CS_ERROR(("epoll ctl del on epoll[%d] fd[%d] failed: [%d:%s]!", _ev_base->_ev_fd, fd, err, strerror(err)));
        }
        else
        {
            CS_INFO(("epoll ctl del on epoll[%d] fd[%d] success", _ev_base->_ev_fd, fd));
        }

        return status;
    }

    int event_wait(int timeout)
    {
        if (_ev_base == NULL)
        {
            return (-1);
        }

        if (timeout < 0)
        {
            timeout = 1;
        }

        while (1)
        {
            int fds = epoll_wait(_ev_base->_ev_fd, _ev_base->_evs, _ev_base->_ev_size, timeout);
            if (fds > 0)
            {
                for (int i = 0; i < fds; ++i)
                {
                    struct epoll_event* ev = &_ev_base->_evs[i];
                    uint32_t events = 0;

                    CS_INFO(("epoll triggered on index[%d], fd[%d]", i, ev->data.fd));

                    if (ev->events & EPOLLERR)
                    {
                        events |= CS_EVENT_ERR;
                    }

                    if (ev->events & (EPOLLIN | EPOLLHUP))
                    {
                        events |= CS_EVENT_READ;
                    }

                    if (ev->events & EPOLLOUT)
                    {
                        events |= CS_EVENT_WRITE;
                    }

                    if (_ev_base->_ev_cb != NULL)
                    {
                        _ev_base->_ev_cb(ev->data.fd, events);
                    }
                }
                return fds;
            }
            else if (fds == 0)
            {
                if (timeout == -1)
                {
                    CS_ERROR(("epoll wait on epoll[%d] with [%d] events and [%d] timeout returned no events", _ev_base->_ev_fd, _ev_base->_ev_size, timeout));
                    return (-1);
                }

                return 0;
            }

            int err = errno;
            if (err == EINTR)
            {
                continue;
            }

            CS_ERROR(("epoll wait on epoll[%d] with [%d] events failed: [%d:%s]!", _ev_base->_ev_fd, _ev_base->_ev_size, err, strerror(err)));
            return (-1);
        }
    }
}

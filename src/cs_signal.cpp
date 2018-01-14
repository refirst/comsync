#include <signal.h>
#include <pthread.h>
#include "cs_util.h"
#include "cs_log.h"
#include "cs_signal.h"

namespace cs_signal
{
    static cs_sig_hup_cb _hup_cb = NULL;
    static cs_sig_usr_cb _usr_cb = NULL;
    static cs_sig_quit_cb _quit_cb = NULL;

    static void unexpected_signal(int sig, siginfo_t* info, void* context)
    {
        UNUSED(context);
        CS_WARN(("[%ld] unexpected signal [%d] caught, signal pid is [%d], code=[%d].", pthread_self(), sig, (int)info->si_pid, info->si_code));

        if (sig == SIGHUP)
        {
            if (_hup_cb != NULL)
            {
                _hup_cb();
            }
        }
        else if (sig == SIGUSR1 || sig == SIGUSR2)
        {
            if (_usr_cb != NULL)
            {
                _usr_cb();
            }
        }
        else if (sig == SIGINT || sig == SIGQUIT || sig == SIGTERM)
        {
            if (_quit_cb != NULL)
            {
                _quit_cb(1);
            }
        }
        else
        {
            exit(1);
        }
    }

    void signal_init()
    {
        //==ignored signals
        signal(SIGCHLD, SIG_IGN);
        signal(SIGPIPE, SIG_IGN);
        signal(SIGALRM, SIG_IGN);
        signal(SIGCLD, SIG_IGN);
#ifdef SIGTSTP
        signal(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
        signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
        signal(SIGTTOU, SIG_IGN);
#endif

        //==unexpected signals
        struct sigaction act;
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = unexpected_signal;
        sigaction(SIGILL, &act, NULL);
        sigaction(SIGKILL, &act, NULL);
        sigaction(SIGFPE, &act, NULL);
        sigaction(SIGBUS, &act, NULL);
        sigaction(SIGSEGV, &act, NULL);
        sigaction(SIGSYS, &act, NULL);
        sigaction(SIGPWR, &act, NULL);
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
#ifdef SIGEMT
        sigaction(SIGEMT, &act, NULL);
#endif
#ifdef SIGSTKFLT
        sigaction(SIGSTKFLT, &act, NULL);
#endif
        sigaction(SIGHUP, &act, NULL);
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);
    }

    void signal_uninit()
    {

    }

    void set_sig_hup_cb(cs_sig_hup_cb cb)
    {
        _hup_cb = cb;
    }

    void set_sig_usr_cb(cs_sig_usr_cb cb)
    {
        _usr_cb = cb;
    }

    void set_sig_quit_cb(cs_sig_quit_cb cb)
    {
        _quit_cb = cb;
    }
}

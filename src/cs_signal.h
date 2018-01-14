#ifndef CS_SIGNAL_H
#define CS_SIGNAL_H

typedef void (*cs_sig_hup_cb)();
typedef void (*cs_sig_usr_cb)();
typedef void (*cs_sig_quit_cb)(int);

namespace cs_signal
{
    void signal_init();
    void signal_uninit();
    void set_sig_hup_cb(cs_sig_hup_cb cb);
    void set_sig_usr_cb(cs_sig_usr_cb cb);
    void set_sig_quit_cb(cs_sig_quit_cb cb);
}

#endif

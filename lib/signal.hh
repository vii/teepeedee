#ifndef _TEEPEEDEE_LIB_SIGNAL_HH
#define _TEEPEEDEE_LIB_SIGNAL_HH

#include <signal.h>
#include <err.h>
#include "unixexception.hh"

class Signal 
{
  sighandler_t _old;
  int _signum;
 public:
  Signal(int sn,sighandler_t n):_signum(sn)
  {
    _old = signal(sn,n);
    if(_old == SIG_ERR)
      throw UnixException("signal");
  }
  ~Signal()
  {
    if(signal(_signum,_old)==SIG_ERR)
      warnx("signal(%d)",_signum);
  }
  
};

#endif

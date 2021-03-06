#ifndef _TEEPEEDEE_LIB_PERMITSIGNALS_HH
#define _TEEPEEDEE_LIB_PERMITSIGNALS_HH

#include <signal.h>

class PermitSignals 
{
  sigset_t _old;
public:
  PermitSignals(int const*masked_signals)
  {
    sigset_t ss;
    sigemptyset(&ss);
    for(const int*s=masked_signals;*s!=NSIG;s++)
      sigaddset(&ss,*s);
    sigprocmask(SIG_UNBLOCK,&ss,&_old);
  }
  ~PermitSignals()
  {
    sigprocmask(SIG_SETMASK,&_old,0);
  }
}
;

#endif

#ifndef _TEEPEEDEE_LIB_IOCONTEXTCONTROLLED_HH
#define _TEEPEEDEE_LIB_IOCONTEXTCONTROLLED_HH

#include <string>
#include <iocontext.hh>

#include "iocontroller.hh"

class IOContextControlled:public virtual IOContext
{
  IOController*_control;
  void notify(bool happy)
  {
    if(!_control)
      return;

    _control->xfer_done(this,happy);
    detach();
  }
public:
  IOContextControlled(IOController*c):_control(c)
  {
  }
  void
  detach()
  {
    _control = 0;
  }
  
  void
  completed(bool happy=true)
  {
    notify(happy);
    delete_this();
  }

  ~IOContextControlled()
  {
    notify(false);
  }
};


#endif

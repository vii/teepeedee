#ifndef _TEEPEEDEE_LIB_XFERLIMITEXCEPTION_HH
#define _TEEPEEDEE_LIB_XFERLIMITEXCEPTION_HH

#include "limitexception.hh"

class XferLimitException:public LimitException{
public:
  const char*
  what()const throw()
  {
    return "transfer limit exceeded";
  }
  ~XferLimitException()throw()
  {
  }
  
};


#endif

#ifndef _TEEPEEDEE_LIB_XFERLIMIT_HH
#define _TEEPEEDEE_LIB_XFERLIMIT_HH

#include <sys/types.h>

#include "xferlimitexception.hh"

class XferLimit{
protected:
  virtual
  bool // true if allowed to xfer that many bytes
  book_xfer(off_t bytes)
  {
    return true;
  };
public:

  void
  xfer(off_t bytes)
  {
    if(!book_xfer(bytes))
      throw XferLimitException();
  }
  
  virtual ~XferLimit()
  {
  }
};


#endif

#ifndef _TEEPEEDEE_SRC_IOCONTROLLER_HH
#define _TEEPEEDEE_SRC_IOCONTROLLER_HH

class IOContextControlled;


class IOController 
{
public:
  virtual ~IOController()
  {
  }
  virtual void
  xfer_done(IOContextControlled*xfer,bool successful)
    =0;
};


#endif

#ifndef _TEEPEEDEE_LIB_IOCONTEXTXFER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTXFER_HH

#include "iocontextcontrolled.hh"
#include "sendfile.hh"

class IOContextXfer : public IOContextControlled,
		      public Sendfile
{
  void
  do_hangup(XferTable&xt)
    ;
  
public:
  IOContextXfer(IOController*ioc,int wfd=-1, int rfd=-1):IOContextControlled(ioc),
						   Sendfile(wfd,rfd)
  {
  }

  
  // this magic for ftp control connections
  // and for http persistant connections
  void
  set_fd(int fd)
  {
    if(get_read_fd() == get_fd())
      set_read_fd(fd);
    if(get_write_fd() == get_fd())
      set_write_fd(fd);
    IOContextControlled::set_fd(fd);
  }

  void
  hangup(XferTable&xt)
    ;

  std::string
  desc()const
  {
    return "sendfile " + IOContextControlled::desc();
  }
  
  events_t
  get_events()
    ;
  bool
  io(const struct pollfd&pfd,XferTable&xt)
    ;
  
  ~IOContextXfer()
  {
    IOContextControlled::set_fd(-1); // stop it being closed twice
  }

};


#endif

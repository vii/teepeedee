#ifndef _TEEPEEDEE_LIB_LISTENER_HH
#define _TEEPEEDEE_LIB_LISTENER_HH

#include <string>

#include <sys/types.h>
#include <arpa/inet.h>

#include "iocontext.hh"

class XferTable;

class Listener:public IOContext
{
  
public:

  // returns true if the sit wrt XT has changed
  virtual
  bool
  new_connection(int newfd,XferTable&xt)
    =0
  ;

  Listener()
  {
  }

  void
  listen();
  
  Listener(uint32_t in_addr,uint16_t port)
  {
    bind_ipv4(in_addr,port);
    listen();
  }

  std::string
  desc()const
    ;
  

  events_t get_events()
    ;
    
  bool io(const struct pollfd&pfd,XferTable&xt)
    ;
  

  ~Listener()
  {
    close();
  }

};
#endif

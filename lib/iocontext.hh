#ifndef _TEEPEEDEE_LIB_IOCONTEXT_HH
#define _TEEPEEDEE_LIB_IOCONTEXT_HH

#include <string>
#include <ctime>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class XferTable;


class IOContext
{
  int _fd;
  std::time_t _max_idle;
  std::time_t _timeout_time;
  
public:

  typedef int events_t;

  IOContext():_fd(-1),_max_idle(0),_timeout_time(0)
  {
  }

  virtual
  bool
  timedout(XferTable&xt)
  {
    hangup(xt);
    return true;
  }
  
  virtual
  events_t get_events()
    =0;

  // returns true if the situation wrt to the XferTable has changed
  virtual
  bool
  io(const struct pollfd&pfd,XferTable&xt)
    =0;

  // Dangerous! should delete THIS!
  virtual
  void
  hangup(XferTable&xt)
    ;
  
  virtual
  std::string
  desc()const
    ;
  
  void
  set_timeout(std::time_t when)
  {
    _timeout_time = when;
  }
  void
  set_timeout_interval(std::time_t i)
  {
    _max_idle = i;
    _timeout_time = 0;
  }

  bool
  has_timeout()const
  {
    return _max_idle;
  }
  
  void
  reset_timeout(std::time_t now)
  {
    if(!has_timeout())
      return;
    set_timeout(now + _max_idle);
  }

  std::time_t
  get_timeout()const
  {
    return _timeout_time;
  }
  
  bool
  active()const
  {
    return get_fd() != -1;
  }
  

  bool
  check_timeout(std::time_t now,XferTable&xt)
  {
    if(!has_timeout())return false;
    std::time_t limit = get_timeout();
    
    if(!limit){
      reset_timeout(now);
      return false;
    }
    if(now > limit){
      reset_timeout(now);
      return timedout(xt);
    }

    return false;
  }
  
  bool
  io_events(const struct pollfd&pfd,std::time_t now,XferTable&xt)
  {
    reset_timeout(now);
    return io(pfd,xt);
  }


  // Dangerous! calls hangup
  void
  discard_hangup(XferTable&xt)
    ;
  
  virtual ~IOContext()
  {
    close();
  }

  int get_fd()const
  {
    return _fd;
  }

  // must not call close()!
  virtual
  void
  set_fd(int fd)
  {
    _fd = fd;
  }

  void
  close();

  void
  become_ipv4_socket()
    ;
  
  // in_addr and port are in network byte order
  void
  bind_ipv4(uint32_t in_addr,uint16_t port)
    ;

  // now in_addr is in network byte order
  // but port_min and port_max are in HOST BYTE ORDER
  void
  bind_ipv4(uint32_t inaddr,uint16_t port_min,uint16_t port_max);

  
  //may throw UnixException
  void
  set_nonblock()
  {
    set_nonblock(get_fd());
  }

  void
  setsockopt(int LEVEL, int OPTNAME, void
	     *OPTVAL, socklen_t OPTLEN)
    ;
  
  void
  set_reuse_addr(bool on=true)
    ;
  
  static
  void
  set_nonblock(int fd)
    ;

  void
  getsockname(struct sockaddr_in&sai)const
    ;
  
  void
  getsockname(void*addr,socklen_t*len)const
    ;

  std::string
  getsockname()const
    ;

  void
  getpeername(struct sockaddr_in&sai)const
    ;
  
  void
  getpeername(void*addr,socklen_t*len)const
    ;

  std::string
  getpeername()const
    ;

  
};


#endif

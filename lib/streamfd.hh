#ifndef _TEEPEEDEE_LIB_STREAMFD_HH
#define _TEEPEEDEE_LIB_STREAMFD_HH
#include <string>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "stream.hh"
#include "unixexception.hh"

class StreamFD : public Stream
{
  int _fd;

  int fcntl(int cmd,int flags)
  {
    return ::fcntl(fd(),cmd,flags);
  }
  void
  set_nonblock()
  {
    int flags = fcntl(F_GETFL,0);
    if(flags==-1)
      throw UnixException("fcntl(F_GETFL)");
    flags |= O_NONBLOCK;
    if(fcntl(F_SETFL,flags)==-1)
      throw UnixException("fcntl(F_SETFL)");
  }
  void
  setsockopt(int LEVEL, int OPTNAME, void
			*OPTVAL, socklen_t OPTLEN)
  {
    if(-1 == ::setsockopt(fd(),LEVEL,OPTNAME,OPTVAL,OPTLEN))
      throw UnixException("setsockopt");
  }
  void
  getsockname(struct sockaddr_in&sai)const
  {
    socklen_t len = sizeof sai;
    getsockname(&sai,&len);
  }
  void
  getsockname(void*addr,socklen_t*len)const
  {
    if(::getsockname(fd(),(sockaddr*)addr,len))
      throw UnixException("getsockname"); //cannot ref desc as that uses getsockname
  }
  std::string
  getsockname()const
    ;
  void
  getpeername(struct sockaddr_in&sai)const
  {
    socklen_t len = sizeof sai;
    getpeername(&sai,&len);
  }
  void
  getpeername(void*addr,socklen_t*len)const
  {
    if(::getpeername(fd(),(sockaddr*)addr,len))
      throw UnixException("getpeername");
  }
  std::string
  getpeername()const
    ;
  
  
  void
  shutdown()
  {
    if(fd()!=-1)
      ::shutdown(fd(),2);
    close();
  }
  
  void
  close()
  {
    if(fd() != -1){
      ::close(fd());
      _fd = -1;
    }
  }
public:
  typedef int events_t;

  std::string
  desc()const
    ;
  
  void
  set_reuse_addr(bool on=true)
  {
    int opt = on ? 1 : 0;
    setsockopt(SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
  }
  
  StreamFD(int f=-1):_fd(-1)
  {
    fd(f);
  }

  Stream*
  accept()const
  {
    int f = ::accept(fd(),0,0);
    if(f == -1){
      if(errno != EWOULDBLOCK)
	throw UnixException("accept");
      return 0;
    }
    try {
      return new StreamFD(f);
    } catch (...){
      ::close(f);
      throw;
    }
  }
  
  // this function should not be used to change the fd while the
  // instance is in a streamtable
  void
  fd(int f)
  {
    close();
    _fd = f;
    if(f!=-1)
      set_nonblock();
  }
  int fd()const
  {
    return _fd;
  }
  virtual
  ssize_t
  read(char*buffer,size_t size)
  {
    ssize_t ret;
    do {
      ret = ::read(fd(),buffer,size);
    }while(ret == -1 && errno == EINTR);
    if(ret == -1){
      if(errno == EAGAIN)
	return -1;
      throw UnixException("read");
    }
    return ret;
  }
  virtual
  ssize_t
  write(const char*buffer,size_t size)
  {
    ssize_t ret;
    do {
      ret = ::write(fd(),buffer,size);
    }while(ret == -1 && errno == EINTR);
    if(ret == -1){
      if(errno == EAGAIN)
	return -1;
      throw UnixException("write");
    }
    return ret;
  }
  ~StreamFD()
  {
    shutdown();
  }
  std::string
  localname()const
  {
    return getsockname();
  }
  std::string
  remotename()const
  {
    return getpeername();
  }
  virtual
  bool
  ready_to_read()
  {
    return false;
  }
  
  virtual
  bool
  ready_to_write()
  {
    return false;
  }

  // XXX implement ready_to_read and ready_to_write with poll or select
};

#endif
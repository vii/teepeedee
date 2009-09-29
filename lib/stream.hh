#ifndef _TEEPEEDEE_LIB_STREAM_HH
#define _TEEPEEDEE_LIB_STREAM_HH

#include <exception>
#include <string>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class IOContext;
class StreamTable;

class Stream 
{
  IOContext*_consumer;
  void free()
    ;
  
  Stream(const Stream&)
  {
  }
  
public:

  class Exception:public std::exception
  {public:
    ~Exception()throw()
    {
    }
  };
  
  class ClosedException:public Exception
  {
  public:
    const char*what()const throw()
    {
      return "attempt to do IO on a closed stream";
    }
    ~ClosedException()throw()
    {
    }
  };
  class UnsupportedException:public Exception
  {
  public:
    const char*what()const throw()
    {
      return "attempt to perform an unsupported operation on a stream";
    }
    ~UnsupportedException()throw()
    {
    }
  };
  
  
  // Bind and listen to a socket
  // Try to find a port randomly in the range [PORT_MIN,PORT_MAX]
  // inclusive
  // INADDR is in network byte order
  // PORT_MIN is in host byte order
  // PORT_MAX is in host byte order
  static Stream*
  listen_ipv4_range(uint32_t inaddr,uint16_t port_min,uint16_t port_max);

  // will never return 0 (throws exception)
  static Stream*
  listen(const sockaddr*addr,socklen_t len)
    ;

  // will never return 0 (throws exception)
  static Stream*
  connect(const sockaddr*addr,socklen_t len,const sockaddr*source_addr=0,socklen_t source_len=0);
  
  Stream(IOContext*c=0):_consumer(c)
  {
  }

  IOContext*
  consumer()
  {
    return _consumer;
  }
  void
  consumer(IOContext*c)
  {
    free();
    _consumer = c;
  }
  void
  release_consumer()
  {
    _consumer=0;
  }
  
  struct State
  {
    enum type { none=0,want_read=1, want_write=2 
    };
  };

  
  virtual
  Stream*
  accept()const
  {
    return 0;
  }
  
  virtual
  ssize_t
  read(char*buffer,size_t size)
    =0;
  virtual
  ssize_t
  write(const char*buffer,size_t size)
   =0;

  virtual
  bool
  ready_to_read()
  {
    return true;
  }
  
  virtual
  bool
  ready_to_write()
  {
    return true;
  }
  
  virtual
  std::string
  localname()const
  {
    return "localend";
  }
  
  virtual
  std::string
  remotename()const
  {
    return "remoteend";
  }
  void
  getsockname(struct sockaddr_in&sai)const
  {
    using std::memset;
    socklen_t len = sizeof sai;
    memset(&sai,0,sizeof sai);
    getsockname(&sai,&len);
  }
  void
  getpeername(struct sockaddr_in&sai)const
  {
    using std::memset;
    socklen_t len = sizeof sai;
    memset(&sai,0,sizeof sai);
    getpeername(&sai,&len);
  }
  
  virtual
  void
  getsockname(void*addr,socklen_t*len)const
  {
    throw UnsupportedException();
  }
  virtual
  void
  getpeername(void*addr,socklen_t*len)const
  {
    throw UnsupportedException();
  }
  virtual
  void
  seek_from_start(off_t pos)
  {
    throw UnsupportedException();
  }

  virtual
  std::string
  desc()const
    =0;

  virtual
  ~Stream()
  {
    free();
  }
};

#endif

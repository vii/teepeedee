#ifndef _TEEPEEDEE_LIB_IOCONTEXT_HH
#define _TEEPEEDEE_LIB_IOCONTEXT_HH

#include <string>
#include <ctime>
#include <sys/types.h>

#include "stream.hh"

// An IOContext is a consumer of IO from a stream

class IOContext
{
  std::time_t _max_idle;
  std::time_t _timeout_time;

  IOContext(const IOContext&)
  {
  }
  void reset_timeout(std::time_t now)
  {
    set_timeout(now + _max_idle);
  }
		     
  void
  set_timeout(std::time_t when)
  {
    _timeout_time = when;
  }
  bool
  has_timeout()const
  {
    return _max_idle;
  }
  
  std::time_t
  get_timeout()const
  {
    return _timeout_time;
  }
  bool
  check_timeout(Stream&stream)
  {
    if(!has_timeout())return false;
    std::time_t limit = get_timeout();
    std::time_t now = std::time(0);
    
    if(!limit){
      reset_timeout(now);
      return false;
    }
    if(now > limit){
      reset_timeout(now);
      timedout(stream);
      return true;
    }
    return false;
  }
protected:  
  void
  made_progress()
  {
    reset_timeout(std::time(0));
  }

  ssize_t
  do_read(Stream&stream,char*buffer,size_t size)
  {
    ssize_t ret = stream.read(buffer,size);
    if(ret != -1)
      made_progress();
    return ret;
  }

  ssize_t
  do_write(Stream&stream,const char*buffer,size_t size)
  {
    ssize_t ret = stream.write(buffer,size);
    if(ret != -1)
      made_progress();
    return ret;
  }

  Stream*
  do_accept(Stream&stream)
  {
    Stream* s = stream.accept();
    if(s)
      made_progress();
    return s;
  }

  virtual
  void
  read_in(Stream&stream,size_t max)
  {
  }

  virtual
  void
  write_out(Stream&stream,size_t max)
  {
  }

  virtual
  void
  timedout(Stream&stream)
  {
    hangup(stream);
  }

  void hangup(Stream&stream)
  {
    if(stream_hungup(stream))
      delete_this();
    else
      if(stream.consumer()==this)
	stream.release_consumer();
  }


public:
  class Destroy
  {
    IOContext*_target;
  public:
    Destroy(IOContext*ioc):_target(ioc)
    {
    }
    IOContext*
    target()
    {
      return _target;
    }
  };
protected:
  void
  delete_this()
  {
    throw Destroy(this);
  }
public:
  IOContext():_max_idle(0),_timeout_time(0)
  {
  }
  
  void
  read(Stream&stream,size_t max=0)
  {
    read_in(stream,max);
  }
  
  void
  write(Stream&stream,size_t max=0)
  {
    write_out(stream,max);
    check_timeout(stream);
  }
  
  // can't make these const as e.g. iocontextwriter needs to do a bunch of stuff
  virtual
  bool
  want_write(Stream&stream)
    =0;
  
  virtual
  bool
  want_read(Stream&stream)
    =0;

  // Notifies IOContext that this related Stream is dead
  // true means that the STREAM was essential to the IOContext and the
  // IOContext sees no further purpose in life in light of the fact
  // that the Stream has died
  virtual
  bool
  stream_hungup(Stream&stream)
  {
    return true;
  }
  
  virtual
  std::string
  desc()const
  {
    return "consumer";
  }

  void
  set_timeout_interval(std::time_t i)
  {
    _max_idle = i;
    _timeout_time = 0;
  }

  virtual
  void // this function is called by IOContextListener when a
       // connection is made and should not be used to do essential
       // stuff
  remote_stream(const Stream&stream)
  {
  }  
		     
  virtual ~IOContext()
  {
  }
};


#endif

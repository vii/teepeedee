#ifndef _TEEPEEDEE_LIB_TIME_HH
#define _TEEPEEDEE_LIB_TIME_HH
#include <iostream>

#include <sys/time.h>
#include "unixexception.hh"

class Time
{
  struct timeval _val;
  void
  reset()
  {
    if(gettimeofday(&_val,0))
      throw UnixException("gettimeofday");
  }
public:
  Time()
  {
    reset();
  }

  void
  strftime(char*buf,size_t size,const char*fmt)const
  {
    if(!::strftime(buf,size,fmt,gmtime(&_val.tv_sec)))
      throw UnixException("strftime");
  }
    
  
  std::string
  xferlog_format()const
  {
    char buf[64];
    strftime(buf,sizeof buf,"%a %b %d %H:%M:%S %Y");
    return std::string(buf);
  }

  long
  microseconds()const
  {
    return _val.tv_usec;
  }
  
  double
  seconds()const
  {
    return double(_val.tv_sec) + double(0.000001)*double(_val.tv_usec);
  }
  
  Time&
  operator-=(Time rhs)
  {
    if(_val.tv_usec < rhs._val.tv_usec) {
      _val.tv_sec --;
      _val.tv_usec += 1000000;
    }
    _val.tv_usec -=  rhs._val.tv_usec;
    _val.tv_sec -= rhs._val.tv_sec;
    return *this;
  }
};

inline
std::ostream&operator<<(std::ostream&o,const Time&t)
{
  return o<<t.xferlog_format();
}

#endif

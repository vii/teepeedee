#ifndef _TEEPEEDEE_UNIXEXCEPTION_HH
#define _TEEPEEDEE_UNIXEXCEPTION_HH

#include <exception>
#include <string>

#include <errno.h>
#include <string.h>

class UnixException : public std::exception
{
  int _errno;
  std::string _desc;
public:
  UnixException(const std::string&d=std::string(), int e = errno):_errno(e),_desc(d)
  {
    if(_desc.empty())_desc="unknown";
    _desc += ": " + std::string(strerror(_errno));
  }

  const char*what() const throw() 
  {
    return _desc.c_str();
  }

  int get_errno()const throw()
  {
    return _errno;
  }
  
  ~UnixException()throw()
  {
  }
  
};

#endif

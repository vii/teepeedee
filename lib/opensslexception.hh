#ifndef _TEEPEEDEE_OPENSSLEXCEPTION_HH
#define _TEEPEEDEE_OPENSSLEXCEPTION_HH

#include <exception>
#include <string>

#include <openssl/err.h>

class OpenSSLException : public std::exception
{
  unsigned long _errno;
  std::string _desc;
public:
  OpenSSLException(const std::string&d=std::string(), int e = ERR_get_error()):_errno(e),_desc(d)
  {
    if(_desc.empty())_desc="unknown";
    _desc += ": " + std::string(ERR_error_string(e,0));
  }

  const char*what() const throw() 
  {
    return _desc.c_str();
  }

  int get_errno()const throw()
  {
    return _errno;
  }
  
  ~OpenSSLException()throw()
  {
  }
  
};

#endif

#ifndef _TEEPEEDEE_LIB_SSLSTREAMFACTORY_HH
#define _TEEPEEDEE_LIB_SSLSTREAMFACTORY_HH

#include <exception>
#include <string>

class Stream;
class StreamContainer;

class SSLStreamFactory
{
  void*_ssl_ctx;//so that we don't need to include the openssl headers
  void free()
    ;
  
public:
  class UnsupportedException:public std::exception
  {
  public:
    const char*what()const throw()
    {
      return "SSL not available";
    }
    ~UnsupportedException()throw()
    {
    }
  };

  
  SSLStreamFactory()
    ;
  
  
  ~SSLStreamFactory()
    ;
  
  Stream*
  new_stream(Stream*source,StreamContainer&table)
    ;
  
  void
  set_keys(const std::string&public_cert_filename, const std::string&private_key_filename)
    ;
  
};
#endif

#ifndef _TEEPEEDEE_LIB_STREAMOPENSSL_HH
#define _TEEPEEDEE_LIB_STREAMOPENSSL_HH

#include "stream.hh"
#include "iocontextopenssl.hh"
#include "opensslexception.hh"

class StreamOpenSSL :public  Stream
{
  typedef Stream super;
  IOContextOpenSSL*_source;

  SSL*
  ssl()
  {
    return _source->ssl();
  }
  bool
  waiting(int ret)
  {
    return _source->waiting(ret);
  }

public:
  StreamOpenSSL(IOContextOpenSSL&fol):_source(&fol)
  {
    _source->accept();
  }
  ~StreamOpenSSL()
  {
    if(_source->hungup())
      delete _source;
    else
      _source->shutdown();
  }

  ssize_t
  read(char*buffer,size_t size)
  {
    int ret;
    ret = SSL_read(ssl(),buffer,size);
    if(ret <0) {
      if(waiting(ret)){
	if(_source->hungup())
	  throw ClosedException();
	return -1;
      }
      throw OpenSSLException("SSL_read");
    }
    return ret;
  }  
  
  virtual
  ssize_t
  write(const char*buffer,size_t size)
  {
    int ret;
    if(_source->hungup())
      throw ClosedException();
    
    if(_source->ssl_write_buffer_too_big())
      return -1;
    ret = SSL_write(ssl(),buffer,size);
    if(ret <0) {
      if(waiting(ret)){
	return -1;
      }
      throw OpenSSLException("SSL_write");
    }
    return ret;
  }

  virtual
  bool
  ready_to_read()
  {
    return SSL_pending(ssl());
  }
  
  virtual
  bool
  ready_to_write()
  {
    return !_source->ssl_write_buffer_too_big();
  }
  
  std::string
  desc()const
  {
    return "SSL/TLS encrypted stream";
  }
};

#endif
  
    

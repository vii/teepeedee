#include "sslstreamfactory.hh"

#ifdef HAVE_OPENSSL
#define _TEEPEEDEE_LIB_IOCONTEXTOPENSSL_I_KNOW_ABOUT_OPENSSL

#include <openssl/bio.h>
#include <openssl/ssl.h>

#include "streamopenssl.hh"
#include "iocontextopenssl.hh"
#include "streamtable.hh"


void
SSLStreamFactory::free()
{
  if(_ssl_ctx){
    SSL_CTX_free((SSL_CTX*)_ssl_ctx);
    _ssl_ctx = 0;
  }
}


SSLStreamFactory::SSLStreamFactory():_ssl_ctx(0)
{
  _ssl_ctx = SSL_CTX_new(SSLv23_server_method());
  if(!_ssl_ctx)
    throw OpenSSLException("SSL_CTX_new");
  
  SSL_CTX_set_default_read_ahead((SSL_CTX*)_ssl_ctx,0);
  SSL_CTX_set_mode((SSL_CTX*)_ssl_ctx,SSL_MODE_ENABLE_PARTIAL_WRITE);
  
  //XXX setting this line is necessary at least for IOContextWriter,
  // but need to check each write to see if it is safe :-(
  SSL_CTX_set_mode((SSL_CTX*)_ssl_ctx,SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
}
  


SSLStreamFactory::~SSLStreamFactory()
{
  free();
}


Stream*
SSLStreamFactory::new_stream(Stream*source,StreamContainer&table)
{
  IOContextOpenSSL*filter = new IOContextOpenSSL((SSL_CTX*)_ssl_ctx);
  try {
    Stream*s= new StreamOpenSSL(*filter);
    source->consumer(filter);
    table.add(source);
    return s;
  } catch (...){
    delete filter;
    throw;
  }
}


void
SSLStreamFactory::set_keys(const std::string&public_cert_filename, const std::string&private_key_filename)
{
  if(SSL_CTX_use_certificate_chain_file((SSL_CTX*)_ssl_ctx,public_cert_filename.c_str())!=1)
    throw OpenSSLException("SSL_CTX_use_certificate_chain_file");

  if(SSL_CTX_use_PrivateKey_file((SSL_CTX*)_ssl_ctx,private_key_filename.c_str(),SSL_FILETYPE_PEM)!=1)
    throw OpenSSLException("SSL_CTX_use_PrivateKey_file");
    
  if(SSL_CTX_check_private_key((SSL_CTX*)_ssl_ctx)!=1)
    throw OpenSSLException("SSL_CTX_check_private_key");
}
#else // HAVE_OPENSSL

SSLStreamFactory::SSLStreamFactory()
{
} 
  
SSLStreamFactory::~SSLStreamFactory()
{
}
  
Stream*
SSLStreamFactory::new_stream(Stream*source,StreamContainer&table)
{
  return source;
}
  
void
SSLStreamFactory::set_keys(const std::string&public_cert_filename, const std::string&private_key_filename)
{
  throw UnsupportedException();
}

#endif // HAVE_OPENSSL

#ifndef _TEEPEEDEE_SRC_SERVERSSL_HH
#define _TEEPEEDEE_SRC_SERVERSSL_HH

#include <server.hh>
#include <sslstreamfactory.hh>

class ServerSSL:public Server,public SSLStreamFactory
{
  typedef Server super;
public:
  
  ServerSSL()
  {
  }
  Stream*
  read_config(const std::string&confname)
  {
    Stream*ret=super::read_config(confname);
    try{
      read_config_ssl();
    } catch (...){
      delete ret;
      throw;
    }
    return ret;
  }
  void
  read_config_ssl()
  {
    std::string pub,priv;
    config().get_fsname("public_cert",pub);
    config().get_fsname("private_key",priv);
    
    set_keys(pub,priv);
  }
  
  
  Stream*
  new_stream(Stream*source,StreamContainer&table)
  {
    return SSLStreamFactory::new_stream(source,table);
  }
  
  std::string desc()const
  {
    return "SSL/TLS " + super::desc();
  }
  
};
#endif

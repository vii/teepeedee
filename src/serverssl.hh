#ifndef _TEEPEEDEE_SRC_SERVERSSL_HH
#define _TEEPEEDEE_SRC_SERVERSSL_HH

#include <server.hh>
#include <sslstreamfactory.hh>

class ServerSSL:public Server,public SSLStreamFactory
{
  typedef Server super;
public:
  
  ServerSSL(Conf&conf):super(conf)
  {
  }
  Stream*
  read_config()
  {
    Stream*ret=super::read_config();
    try{
      read_config_ssl();
    } catch (...){
      ret->release_consumer();
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

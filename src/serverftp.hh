#ifndef _TEEPEEDEE_SRC_SERVERFTP_HH
#define _TEEPEEDEE_SRC_SERVERFTP_HH

#include <exception>

#include <err.h>

#include "server.hh"
#include "serverssl.hh"

class ServerFTP : public ServerSSL
{
  typedef ServerSSL super;
  bool _ssl_available;
public:
  ServerFTP(Conf&c):ServerSSL(c),_ssl_available(false)
  {
  }
  
  IOContext*
  new_iocontext()
    ;

  static
  Server*
  factory(Conf&conf)
  {
    return new ServerFTP(conf);
  }

  std::string desc()const
  {
    return "ftp " + Server::desc();
  }

  Stream*
  new_stream(Stream*s,StreamContainer&)
  {
    return s;
  }
  
  Stream*
  read_config()
  {
    _ssl_available = false;
    Stream*ret=Server::read_config();
    try{
      read_config_ssl();
      _ssl_available = true;
    } catch (const SSLStreamFactory::UnsupportedException&ue){
    } catch (const std::exception&e){
      warnx("SSL not available for server at %s because %s",config().get_name().c_str(),e.what());
    } catch (...){
    }
    return ret;
  }
  
};


#endif

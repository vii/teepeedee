#ifndef _TEEPEEDEE_SRC_SERVERHTTPS_HH
#define _TEEPEEDEE_SRC_SERVERHTTPS_HH

#include "serverhttp.hh"
#include "serverssl.hh"

class ServerHTTPS : public ServerSSL 
{
  typedef ServerSSL super;
public:
  ServerHTTPS(Conf&c):super(c)
  {
  }
  
  static
  Server*
  factory(Conf&c)
  {
    return new ServerHTTPS(c);
  }
  IOContext*
  new_iocontext()
    ;

  std::string desc()const
  {
    return "http " + super::desc();
  }
};

#endif

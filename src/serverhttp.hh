#ifndef _TEEPEEDEE_SRC_SERVERHTTP_HH
#define _TEEPEEDEE_SRC_SERVERHTTP_HH

#include "server.hh"

class ServerHTTP : public Server
{
public:
  ServerHTTP(Conf&c):Server(c)
  {
  }
  
  IOContext*
  new_iocontext()
    ;

  static
  Server*
  factory(Conf&c)
  {
    return new ServerHTTP(c);
  }

  std::string desc()const
  {
    return "http " + Server::desc();
  }

};


#endif

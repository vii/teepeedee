#ifndef _TEEPEEDEE_SRC_SERVERHTTP_HH
#define _TEEPEEDEE_SRC_SERVERHTTP_HH

#include "server.hh"

class ServerHTTP : public Server
{
public:

  bool
  new_connection(int newfd,XferTable&xt)
    ;

  static
  Server*
  factory()
  {
    return new ServerHTTP;
  }

  std::string desc()const
  {
    return "http " + Server::desc();
  }
  
};


#endif

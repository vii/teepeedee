#ifndef _TEEPEEDEE_SRC_SERVERFTP_HH
#define _TEEPEEDEE_SRC_SERVERFTP_HH

#include "server.hh"

class ServerFTP : public Server
{
public:

  bool
  new_connection(int newfd,XferTable&xt)
    ;

  static
  Server*
  factory()
  {
    return new ServerFTP;
  }

  std::string desc()const
  {
    return "ftp " + Server::desc();
  }
  
};


#endif

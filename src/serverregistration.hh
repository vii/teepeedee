#ifndef _TEEPEEDEE_SRC_SERVERREGISTRATION_HH
#define _TEEPEEDEE_SRC_SERVERREGISTRATION_HH

#include <string>
#include <list>
#include "server.hh"

class ServerRegistration 
{
  std::string _name;
  Server::Factory _factory;
  std::string _desc;
  ServerRegistration*_next;
public:
  static ServerRegistration*registered_list;
  
  ServerRegistration(const std::string&n,Server::Factory f,const std::string&d)
    :_name(n),
     _factory(f),
     _desc(d)
  {
    if(!registered_list){
      registered_list = this;
      return;
    }
    ServerRegistration*parent;
    for(parent=registered_list;parent->_next;parent=parent->_next);
    parent->_next = this;
  }
  std::string name()const
  {
    return _name;
  }
  Server::Factory factory()const
  {
    return _factory;
  }
  ServerRegistration*
  next()
  {
    return _next;
  }
};
#endif

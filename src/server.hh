#ifndef _TEEPEEDEE_SRC_SERVER_HH
#define _TEEPEEDEE_SRC_SERVER_HH

#include <conftree.hh>
#include <listener.hh>

class Server:public Listener
{
  ConfTree _conf;
protected:

  ConfTree&
  config()
  {
    return _conf;
  }
public:

  // throws exception on failure
  void
  read_config(const std::string&confname)
    ;

  typedef Server* (*Factory)();
  
};


#endif

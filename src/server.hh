#ifndef _TEEPEEDEE_SRC_SERVER_HH
#define _TEEPEEDEE_SRC_SERVER_HH

#include <conftree.hh>
#include <iocontextlistener.hh>

class Server:public IOContextListener
{
  typedef IOContextListener super;
  
  ConfTree _conf;
//   bool
//   full()
//   {
//     return !config().book_increment("unauthenticated_connections",0);
//   }
//   bool
//   increment()
//   {
//     return config().book_increment("unauthenticated_connections",1);
//   }
//   void
//   decrement()
//   {
//     return config().book_decrement("unauthenticated_connections",1);
//   }
  
protected:
  ConfTree&
  config()
  {
    return _conf;
  }    
  
public:

  // throws exception on failure
  virtual
  Stream*
  read_config(const std::string&confname)
    ;

  typedef Server* (*Factory)();
};


#endif

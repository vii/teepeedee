#ifndef _TEEPEEDEE_SRC_SERVER_HH
#define _TEEPEEDEE_SRC_SERVER_HH

#include <conf.hh>
#include <iocontextlistener.hh>

class Server:public IOContextListener
{
  typedef IOContextListener super;
  
  Conf _conf;
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
  Conf&
  config()
  {
    return _conf;
  }    
  
public:
  Server(Conf&conf):_conf(conf)
  {
  }
  
  typedef Server* (*Factory)(Conf&conf);

  virtual
  Stream*
  read_config()
    ;
};


#endif

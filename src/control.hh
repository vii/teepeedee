#ifndef _TEEPEEDEE_SRC_CONTROL_HH
#define _TEEPEEDEE_SRC_CONTROL_HH

#include <string>

#include <iocontroller.hh>
#include <iocontextresponder.hh>
#include <conf.hh>

class StreamContainer;
class User;

class Control : public IOContextResponder, public IOController
{
  typedef IOContextResponder super;
  StreamContainer&_stream_container;
  Conf _conf;
  bool _authenticated;

  void
  decrement()
    ;
  void
  increment();
  
protected:
  Conf&config(){return _conf;
  }
  StreamContainer&
  stream_container()
  {
    return _stream_container;
  }  
public:

  Control(const Conf&conf,StreamContainer&s):_stream_container(s),
						 _conf(conf),_authenticated(false)
  {
    increment();//yes may throw exception
  }

  bool
  user_login_default(User&user,const std::string&password=std::string())
    ;
  
  bool
  user_login(User&user,const std::string&username,const std::string&password)
    ;
  
  std::string
  desc()const
  {
    return "protocol control connection " + super::desc();
  }
  ~Control()
  {
    decrement();
  }
};

#endif

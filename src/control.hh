#ifndef _TEEPEEDEE_SRC_CONTROL_HH
#define _TEEPEEDEE_SRC_CONTROL_HH

#include <string>

#include <iocontroller.hh>
#include <iocontextresponder.hh>
#include <conftree.hh>

class StreamContainer;
class User;

class Control : public IOContextResponder, public IOController
{
  typedef IOContextResponder super;
  StreamContainer&_stream_container;
  ConfTree _conf;
  bool _authenticated;
protected:
  ConfTree&config(){return _conf;
  }
  StreamContainer&
  stream_container()
  {
    return _stream_container;
  }
public:

  Control(const ConfTree&conf,StreamContainer&s):_stream_container(s),
						 _conf(conf),_authenticated(false)
  {
  }

  bool
  user_login(User&user,const std::string&username,const std::string&password)
    ;
  
  std::string
  desc()const
  {
    return "protocol control connection " + super::desc();
  }

  void
  decrement()
    ;

  ~Control()
  {
    decrement();
  }
};

#endif

#include <err.h>
#include "control.hh"
#include "user.hh"

void
Control::increment()
{
  if(!config().book_increment("unauthenticated_connections"))
    throw LimitException();
}

void
Control::decrement()
{
      if(!_authenticated) {
	config().book_decrement("unauthenticated_connections");
	_authenticated = true;
      }
}

bool
Control::user_login_default(User&user,const std::string&password)
{
  return user_login(user,"default-user",password);
}

bool
Control::user_login(User&user,const std::string&username,const std::string&password)
{
  try{
    Conf users(config(),"users");
    
    if(user.authenticate(username,password,users)){
      decrement();
      return true;
    }
    
  } catch(LimitException&){
    throw;
  }
  catch (std::exception&e){
    warnx("error for user trying to login: %s",e.what());
  }

  return false;
}

  

#include <err.h>
#include "control.hh"
#include "user.hh"

void
Control::decrement()
{
      if(!_authenticated) {
	config().book_decrement("unauthenticated_connections",1);
	_authenticated = true;
      }
}

bool
Control::user_login(User&user,const std::string&username,const std::string&password)
{
  try{
    ConfTree users;
    config().get("users",users);
    
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

  

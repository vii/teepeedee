#include <xfertable.hh>

#include "serverhttp.hh"
#include "httpcontrol.hh"

bool
ServerHTTP::new_connection(int newfd,XferTable&xt)
{
  HTTPControl*httpc = new HTTPControl(newfd,config());
  
  xt.add(httpc);
  return true;
}

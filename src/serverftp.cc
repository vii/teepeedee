#include <xfertable.hh>

#include "serverftp.hh"
#include "ftpcontrol.hh"

bool
ServerFTP::new_connection(int newfd,XferTable&xt)
{
  FTPControl*ftpc = new FTPControl(newfd,config());
  
  xt.add(ftpc);
  return true;
}

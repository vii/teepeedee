#include <cstring>
#include <sys/poll.h>
#include <err.h>

#include <unixexception.hh>
#include <iocontext.hh>
#include <xfertable.hh>

#include "ftpdatalistener.hh"
#include "ftpcontrol.hh"

void
FTPDataListener::set_data(IOContext*fdc)
{
  _data = fdc;
}

FTPDataListener::events_t FTPDataListener::get_events()
{
  if(_data)
    return POLLIN;
  return 0;
}

bool
FTPDataListener::new_connection(int newfd,XferTable&xt)
{
  if(!_data){
    warnx("internal error in FTPDataListener::new_connection");
    return true; // true as XT should be changed not to talk to us as we have no _DATA
  }

  _data->set_fd(newfd);
  xt.add(_data);
  _data = 0;
  return true;
}


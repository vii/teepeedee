#include <cstring>
#include <sys/poll.h>
#include <err.h>

#include <unixexception.hh>
#include <iocontext.hh>

#include "ftpdatalistener.hh"

void
FTPDataListener::set_data(IOContext*fdc)
{
  _data = fdc;
}

IOContext*
FTPDataListener::new_iocontext()
{
  if(!has_data()){
    delete_this();
    return 0;
  }
  IOContext*tmp = _data;
  release_data();
  return tmp;
}


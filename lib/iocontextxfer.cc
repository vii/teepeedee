#include <sys/poll.h>
#include <err.h>

#include "unixexception.hh"
#include "iocontextxfer.hh"

IOContextXfer::events_t
IOContextXfer::get_events()
{
  if(get_fd() == get_read_fd())
    return POLLIN;
  if(get_fd() == get_write_fd())
    return POLLOUT;
  return 0;
}

bool
IOContextXfer::io(const struct pollfd&pfd,XferTable&xt)
{
  try{
    if(Sendfile::io()){
      successful();
      hangup(xt);
      return true;
    }
  } catch (UnixException&ue){
    warnx("stopping xfer: %s",ue.what());
    do_hangup(xt);
    return true;
  }
  return false;
}


void
IOContextXfer::do_hangup(XferTable&xt)
{
  IOContextControlled::hangup(xt);
}
void
IOContextXfer::hangup(XferTable&xt)
{
  if(get_write_fd() != get_fd()){
    try{
      while(data_buffered())
	if(Sendfile::io()) {
	  successful();
	  break;
	}
      // have to do one more to see if we've completed
      if(Sendfile::io()) {
	successful();
      }
    } catch (const UnixException&ue) {
      warnx("flushing sendfile buffer: %s",ue.what());
    }
  }
  do_hangup(xt);
}

#include <sys/poll.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

#include "iocontextwriter.hh"

IOContextWriter::events_t IOContextWriter::get_events()
{
  return POLLOUT;
}


bool IOContextWriter::io(const struct pollfd&pfd,class XferTable&xt)
{
  // the infinite loop round here is too strong and can mean that
  // retrieves totally take over the server
  
  //  for(;;){
    if(write_buf_empty()){
      finished_writing(xt);
      if(write_buf_empty())
	return true;
    }
    
    size_t len = _buf_len - _buf_pos;
    ssize_t ret = write(get_fd(),_buf+_buf_pos,len);
    if(ret == -1){
      if(errno == EAGAIN)
	return false;
      if(errno != ECONNRESET)
	warn("write for %s",desc().c_str());
      hangup(xt);
      return true;
    }
    _buf_pos += ret;

    //  }

    // need to tell producer as soon as writing finished or else it gets
    // confused (FTPControl)
    if(write_buf_empty())
      finished_writing(xt);
    return write_buf_empty();
}


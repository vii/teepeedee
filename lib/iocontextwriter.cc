#include <sys/poll.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

#include "unixexception.hh"
#include "iocontextwriter.hh"

void
IOContextWriter::write_out(Stream&stream,size_t max)
{
  if(write_buf_empty()){
    do_finished_writing();
    if(write_buf_empty())
      return;
  }
  
  // the infinite loop round here is too strong and can mean that
  // retrieves totally take over the server
  
  //  for(;;){
  size_t len = _buf_len - _buf_pos;
  ssize_t ret;
  ret = do_write(stream,_buf+_buf_pos,len);
  _buf_pos += ret;

  //  }

  // need to tell producer as soon as writing finished or else it gets
  // confused (FTPControl)
  if(write_buf_empty())
    do_finished_writing();
}

#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "unixexception.hh"
#include "sendfile.hh"
#include "xferlimit.hh"
#include "streamfd.hh"

#ifndef HAVE_SENDFILE
static
ssize_t
system_sendfile(int wfd, int rfd)
{
  return -1;
}
#else
#ifdef HAVE_SYS_SENDFILE_H

// This works on Linux at least
#include <sys/sendfile.h>

static
ssize_t
system_sendfile(int wfd, int rfd)
{
  // XXX check up: suppose it returns 0 but in fact has something left to read?
  ssize_t ret = sendfile(wfd,rfd,0,(unsigned)-1);  

  if(ret == -1 && errno != EINVAL && errno != ENOSYS)
    throw UnixException("sendfile");
  
  return ret;
}

#else
#ifdef HAVE_SYS_UIO_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>

// This one works at least on FreeBSD
static
ssize_t
system_sendfile(int wfd, int rfd)
{
  off_t pos = lseek(rfd,0,SEEK_CUR);
  if(pos == -1)
    return -1;
  
  off_t written = 0;
  int ret = sendfile(rfd,wfd,pos,0,0,&written,0);
  
  if(ret == -1) {
    if(errno == EAGAIN) {
      if(lseek(rfd,written,SEEK_CUR) == -1)
	throw UnixException("lseek after sendfile");
      return written;
    }
    if(errno != EINVAL && errno != ENOTSOCK)
      throw UnixException("sendfile");
    return -1;
  }
  
  if(lseek(rfd,written,SEEK_CUR) == -1)
    throw UnixException("lseek after sendfile complete");
  
  return 0;
}

#endif
#endif
#endif

bool
Sendfile::io(XferLimit*limit)
{
  if(!_buf) {
    StreamFD*in,*out;
    if((in=dynamic_cast<StreamFD*>(_in))&&(out=dynamic_cast<StreamFD*>(_out))){
      ssize_t ret;
      ret = system_sendfile(out->fd(),in->fd());
      if(ret == 0)
	return true;
      if(ret != -1){
	if(limit)limit->xfer(ret);
	return false;
      }
    
    }
    _buf = new Buf;
  }

  if(data_buffered()){
    write_out();
    if(data_buffered())
      return false;
  }

  bool ret = read_in();
  if(limit)limit->xfer(_buf->len);
  
  if(data_buffered())
    write_out();
  
  return ret && !data_buffered();
}

void
Sendfile::write_out()
{
  ssize_t ret;
  for(;;){
    ret = _out->write(_buf->buffer + _buf->pos,_buf->len - _buf->pos);
    if(ret == -1)
      return;
    _buf->pos += ret;
    return;
  }
}


bool
Sendfile::read_in()
{
  ssize_t ret;
  _buf->pos = 0;
  _buf->len = 0;
  for(;;){
    ret  = _in->read(_buf->buffer,sizeof _buf->buffer);
    if(ret == 0)
      return true;
    if(ret == -1)
      return false;
    _buf->len = ret;
    return false;
  }
}


#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "unixexception.hh"
#include "sendfile.hh"

#include "config.h"

#ifdef HAVE_SYS_SENDFILE_H

// This works on Linux at least
#include <sys/sendfile.h>

static
ssize_t
system_sendfile(int wfd, int rfd)
{
  // XXX check up: suppose it returns 0 but in fact has something left to read?
  ssize_t ret = sendfile(wfd,rfd,0,(unsigned)-1);  

  if(ret == -1 && errno != EINVAL)
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
      return 1;
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

bool
Sendfile::io()
{
  ssize_t ret;

  if(!_buf){
    ret = system_sendfile(_write_fd,_read_fd);
    if(ret == 0)
      return true;
    if(ret != -1)
      return false;

    _buf = new Buf;
  }

  if(data_buffered()){
  redo_write:
    ret = write(_write_fd,_buf->buffer + _buf->pos,_buf->len - _buf->pos);
    if(ret == -1){
      if(errno == EINTR)
	goto redo_write;

      if(errno == EAGAIN){
	return false;
      }
      throw UnixException("write");
    }
    _buf->pos += ret;
    return false;
  }

 redo_read:
  ret  = read(_read_fd,_buf->buffer,sizeof _buf->buffer);
  if(ret == 0)
    return true;
  if(ret == -1){
    if(errno == EAGAIN)
      return false;
    if(errno == EINTR)
      goto redo_read;

    throw UnixException("read");
  }
  _buf->len = ret;
  _buf->pos = 0;
  return false;
}

void
Sendfile::close()
{
  if(_read_fd != -1) {
    if(::close(_read_fd))
      warn("close on sendfile read filedescriptor");

    _read_fd = -1;
  }
  if(_write_fd != -1) {
    if(::close(_write_fd))
      warn("close on sendfile write filedescriptor");
      
    _write_fd = -1;
  }
  if(_buf){
    delete _buf;
    _buf = 0;
  }
}

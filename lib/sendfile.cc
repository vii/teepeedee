#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "unixexception.hh"
#include "sendfile.hh"
#include "streamfd.hh"

#if defined(HAVE_SENDFILE)&&defined(HAVE_DECL_SENDFILE)&& HAVE_DECL_SENDFILE
#ifdef HAVE_SYS_SENDFILE_H

#define SENDFILE_FUNCTION_PRESENT

// This works on Linux at least
#include <sys/sendfile.h>

static
ssize_t
system_sendfile(int wfd, int rfd)
{
  // XXX check up: suppose it returns 0 but in fact has something left to read?
  ssize_t ret = sendfile(wfd,rfd,0,(unsigned)-1);  

  if(ret == -1){
    if(errno == EAGAIN)
      return ret;
    throw UnixException("sendfile");
  }
  return ret;
}

#else
#ifdef HAVE_SYS_UIO_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>

#define SENDFILE_FUNCTION_PRESENT

// This one works at least on FreeBSD
static
ssize_t
system_sendfile(int wfd, int rfd)
{
  off_t pos = lseek(rfd,0,SEEK_CUR);
  if(pos==-1)
    throw UnixException("lseek before sendfile");
  
  off_t written = 0;
  int ret = sendfile(rfd,wfd,pos,0,0,&written,0);
  
  if(ret == -1) {
    if(errno == EAGAIN) {
      if(lseek(rfd,written,SEEK_CUR) == -1)
	throw UnixException("lseek after sendfile");
      return written?written:-1;
    }
    throw UnixException("sendfile");
  }
  
  if(lseek(rfd,written,SEEK_CUR) == -1)
    throw UnixException("lseek after sendfile complete");
  
  return written;//here ret==0 so the sendfile is complete - should be able to inform
}

#endif
#endif
#endif

bool
Sendfile::io()
{
  if(!_buf) {
#ifdef SENDFILE_FUNCTION_PRESENT
    StreamFD*in,*out;
    if((in=dynamic_cast<StreamFD*>(_in))&&(out=dynamic_cast<StreamFD*>(_out))){
      ssize_t ret;
      try{
	ret = system_sendfile(out->fd(),in->fd());
	if(ret == 0) {
	  return true;
	}
	if(ret != -1){
	  limit_xfer(ret);
	}
	return false;
      } catch (const UnixException&ue){
      }
    }
#endif    
    _buf = new Buf;
  }

  if(data_buffered()){
    write_out();
    if(data_buffered())
      return false;
  }

  bool ret = read_in();
  limit_xfer(_buf->len);

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
    try{
      ret  = _in->read(_buf->buffer,sizeof _buf->buffer);
    } catch (const Stream::ClosedException&){
      return true;
    }
    if(ret == -1)
      return false;
    _buf->len = ret;
    return false;
  }
}


#include <iostream>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>
#include <err.h>

#include "iocontextresponder.hh"

void
IOContextResponder::read_done(XferTable&xt)
{
  std::cerr << "<< " << getpeername()  << ": " << std::string(_buf,_buf_write_pos) << std::endl;
  finished_reading(xt,_buf,_buf_write_pos);
  _buf_write_pos = 0;
}

bool IOContextResponder::io(const struct pollfd&pfd,class XferTable&xt)
{
  if(response_ready())
    return IOContextWriter::io(pfd,xt);

  if(_closing) {
    hangup(xt);
    return true;
  }
    
  read_in(xt);
  return response_ready();
}

void
IOContextResponder::prepare_io()
{
  if(response_ready()){
    if(write_buf_empty())
      set_write_buf(_response.c_str(),_response.length());
  }
  else
    prepare_read();
}

void
IOContextResponder::read_in(XferTable&xt)
{
  char*end = _buf + sizeof(_buf);
  
  for(char*start = _buf + _buf_write_pos;start < end; ++start,++_buf_write_pos){
    // XXX optimize to reduce number of syscalls
    ssize_t retval = read(get_fd(),start,1);
    if(retval == 0){
      *start = 0;
      read_done(xt);
      return;
    }
    if(retval == -1){
      if(errno == EAGAIN)
	return;
      warn("read failed");
      hangup(xt);
      return;
    }
    
    if(retval == 1){
      if(*start == '\n'){
	// permit both \r\n and \n for Mr Plato
	*start = 0;

	if(_buf_write_pos && *(start-1) == '\r'){
	  *(start-1) = 0;
	  _buf_write_pos--;
	}
	read_done(xt);
	return;
      }
    }
  }

  _buf_write_pos = 0;
  input_line_too_long(xt);
}


void
IOContextResponder::finished_writing(class XferTable&xt)
{
  std::cerr << ">> " << getpeername()  << ": " << _response << std::flush;
  _response = std::string();
  prepare_io();
}

IOContextResponder::events_t
IOContextResponder::get_events()
{
  return response_ready() ? POLLOUT : (closing() ? POLLOUT|POLLIN : POLLIN);
}

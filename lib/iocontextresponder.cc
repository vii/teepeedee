#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <errno.h>
#include <err.h>
#include <time.h>
#include <assert.h>

#include "iocontextresponder.hh"

void
IOContextResponder::read_done(Stream&stream)
{
  std::cerr << "<< " << stream.remotename() << ": " << std::string(_buf,_buf_write_pos) << std::endl;
  unsigned tmp = _buf_write_pos;
  prepare_read();
  //might throw exception
  finished_reading(_buf,tmp);
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
IOContextResponder::write_out(Stream&stream,size_t max)
{
  if(!response_ready())
    return;
  return super::write_out(stream,max);
}


void
IOContextResponder::read_in(Stream&stream,size_t max)
{
  if(response_ready())
    return;
  if(closing()){
    delete_this();
  }

  for(char*start = _buf + _buf_write_pos;_buf_write_pos < sizeof _buf;
      ++start,
      ++_buf_write_pos
      ){
    // XXX optimize to reduce number of syscalls
    ssize_t retval;
    retval = do_read(stream,start,1);

    switch(retval){
    case -1:
      return;
    case 0:
      *start = 0;
      hangup(stream);
      return;
    case 1:
      if(*start == '\n'){
	// permit both \r\n and \n for Mr Plato
	*start = 0;
	
	if(_buf_write_pos && *(start-1) == '\r'){
	  *(start-1) = 0;
	  _buf_write_pos--;
	}
	read_done(stream);
	return;
      }
      break;
    }
  }

  _buf_write_pos = 0;
  input_line_too_long();
}

void
IOContextResponder::report_connect()
{
  struct timeval tv;
  char buf[50];
  
  if(gettimeofday(&tv,0))
    warn("gettimeofday");

  time_t t = tv.tv_sec;
  if(!strftime(buf,sizeof buf,"%a, %d %b %Y %H:%M:%S",gmtime(&t))){
    warn("strftime");
    buf[0]=0;
  }
  
  std::cerr << "+++ " 
	    << " connecting to " << desc()
	    << " at " << buf << '.'
	    << std::setw(6) << std::setfill('0') << tv.tv_usec
	    << std::endl;
}

void
IOContextResponder::finished_writing()
{
  std::cerr << ">> : " << _response << std::flush;
  _response = std::string();
  prepare_io();
}

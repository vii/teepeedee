#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#include "time.hh"
#include "iocontextresponder.hh"

void
IOContextResponder::read_done(Stream&stream)
{
  std::cerr << "<< " << remotename() << ": " << std::string(_buf,_buf_write_pos) << std::endl;
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
IOContextResponder::report(bool connecting)
{
  Time now;
  char buf[50];
  now.strftime(buf,sizeof buf,"%a, %d %b %Y %H:%M:%S");
  
  std::cerr << (connecting ?"+++ ": "--- ") << remotename() << ' ';
  if(connecting)
    std::cerr << "connecting to " << desc();
  else
    std::cerr <<"closed";
  
  std::cerr<< " at " << buf << '.'
	   << std::setw(6) << std::setfill('0') << now.microseconds()
	   << std::endl;
}

void
IOContextResponder::finished_writing()
{
  std::cerr << ">> " <<  remotename() << ": " <<_response << std::flush;
  _response = std::string();
  prepare_io();
}

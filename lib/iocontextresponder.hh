#ifndef _TEEPEEDEE_LIB_IOCONTEXTRESPONDER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTRESPONDER_HH

#include <list>
#include <string>

#include "iocontextwriter.hh"

// a class for protocols that read in one line then output something

class IOContextResponder : public IOContextWriter 
{
  std::string _response;
  char _buf[400];
  unsigned _buf_write_pos;
  bool _closing;

  void
  read_in(XferTable&xt)
    ;

  void
  prepare_io()
    ;
  
  void prepare_read()
  {
    _buf_write_pos = 0;
  }
  
protected:
  void
  close_after_output()
  {
    _closing = true;
  }
  bool
  closing()const
  {
    return _closing;
  }
  
  bool response_ready()const
  {
    return !_response.empty();
  }
  void
  finished_writing(XferTable&xt)
    ;

  void
  read_done(XferTable&xt)
    ;
  
  virtual
  void
  finished_reading(XferTable&xt,char*buf,size_t len)
    =0;
  
  virtual
  void
  input_line_too_long(XferTable&xt)
  {
    hangup(xt);
  }

  void
  add_response(const std::string&resp)
  {
    _response += resp;
    if(!write_buf_empty())
      move_write_buf(_response.c_str(),_response.length());
    else
      prepare_io();
  }
public:
  IOContextResponder():
    _buf_write_pos(0),
    _closing(false)
  {
  }
  
  
  bool
  io(const struct pollfd&pfd,class XferTable&xt)
    ;

  events_t get_events();
  
};
#endif

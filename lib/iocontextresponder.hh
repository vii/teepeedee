#ifndef _TEEPEEDEE_LIB_IOCONTEXTRESPONDER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTRESPONDER_HH

#include <list>
#include <string>

#include "iocontextwriter.hh"

// a class for protocols that read in one line then output something

class IOContextResponder : public IOContextWriter 
{
  typedef IOContextWriter super;

  std::string _remotename;
  std::string _response;
  char _buf[2000];
  unsigned _buf_write_pos;
  bool _closing;

  void
  report(bool connecting=true)
    ;
  

  void
  prepare_io()
    ;
  
  void prepare_read()
  {
    _buf_write_pos = 0;
  }
  

protected:
  ~IOContextResponder()
  {
    report(false);
  }
  
  void
  read_in(Stream&stream,size_t max)
    ;
  void
  write_out(Stream&stream,size_t max)
    ;

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
  finished_writing()
    ;

  void
  read_done(Stream&stream)
    ;
  
  virtual
  void
  finished_reading(char*buf,size_t len)
    =0;
  virtual
  void
  input_line_too_long()
  {
    delete_this();
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
  std::string
  remotename()const
  {
    return _remotename;
  }
public:
  IOContextResponder():
    _buf_write_pos(0),
    _closing(false)
  {
  }
  bool
  want_write(Stream&stream)
  {
    return response_ready() || closing();
  }
  bool
  want_read(Stream&stream)
  {
    if(!response_ready())
      return true;
    return false;
  }
  void
  remote_stream(const Stream&stream)
  {
    _remotename = stream.remotename();
    report();
  }
  
};
#endif

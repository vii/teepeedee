#ifndef _TEEPEEDEE_LIB_IOCONTEXTWRITER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTWRITER_HH

#include "iocontext.hh"

class IOContextWriter : public virtual IOContext
{
  const char* _buf;
  unsigned _buf_len;
  unsigned _buf_pos;

  void
  do_finished_writing()
  {
    finished_writing();
  }
  
protected:
  unsigned
  buf_len()const
  {
    return _buf_len;
  }
  
  bool
  write_buf_empty()const
  {
    return !_buf || !_buf_len || _buf_pos >= _buf_len;
  }
  virtual
  void
  finished_writing()
  {
  }

  void reset_write_buf()
  {
    _buf = 0;
    _buf_pos = 0;
    _buf_len = 0;
  }
  void set_write_buf(const char*buf,unsigned len)
  {
    move_write_buf(buf,len);
    _buf_pos = 0;
  }
  void move_write_buf(const char*buf,unsigned len)
  {
    _buf = buf;
    _buf_len = len;
  }

  bool
  want_write(Stream&stream)
  {
    if(write_buf_empty()){
      do_finished_writing();
    }
    return !write_buf_empty();
  }
  bool
  want_read(Stream&stream)
  {
    return false;
  }
protected:
  void // return true if want to write more
  write_out(Stream&stream,size_t max)
    ;
  
public:
  IOContextWriter():_buf(0),_buf_len(0),_buf_pos(0)
  {
  }

};

#endif

#ifndef _TEEPEEDEE_LIB_SENDFILE_HH
#define _TEEPEEDEE_LIB_SENDFILE_HH

#include "stream.hh"

class Sendfile 
{
  Stream*_in;
  Stream*_out;

  bool
  read_in();

  void
  free()
  {
    if(_buf){
      delete _buf;
      _buf = 0;
    }
  }
  
  struct Buf
  {
    char buffer[1<<24];
    unsigned len;
    unsigned pos;

    Buf():len(0),pos(0)
    {
    }
  };
  Buf*_buf;

  Sendfile(const Sendfile&)
  {
  }
protected:
  bool
  has_buf()const
  {
    return _buf;
  }

  virtual
  void
  limit_xfer(off_t bytes)
  {
  }
  
  
public:
  Sendfile(Stream*in,Stream*out):
    _in(in),_out(out),
    _buf(0)
  {
  }

  Stream*
  stream_in()
  {
    return _in;
  }
  Stream*
  stream_out()
  {
    return _out;
  }
  void
  stream_in(Stream*s)
  {
    _in=s;
  }
  void
  stream_out(Stream*s)
  {
    _out=s;
  }
  
  bool
  data_buffered()
  {
    if(!_buf)
      return false;
    return (_buf->pos < _buf->len);
  }
  void
  write_out();
  
  // returns true if finished (may throw XferLimitException)
  bool
  io();

  ~Sendfile()
  {
    free();
  }
};

#endif

#ifndef _TEEPEEDEE_LIB_IOCONTEXTWRITER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTWRITER_HH

#include "iocontext.hh"

class IOContextWriter : public virtual IOContext
{
  const char* _buf;
  unsigned _buf_len;
  unsigned _buf_pos;

  
protected:
  bool
  write_buf_empty()const
  {
    return !_buf || !_buf_len || _buf_pos >= _buf_len;
  }
  virtual
  void
  finished_writing(class XferTable&xt)
  {
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
public:
  IOContextWriter():_buf(0),_buf_len(0),_buf_pos(0)
  {
  }

  events_t get_events()
    ;
  
  bool io(const struct pollfd&pfd,class XferTable&xt)
    ;
};

#endif

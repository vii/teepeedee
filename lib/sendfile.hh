#ifndef _TEEPEEDEE_LIB_SENDFILE_HH
#define _TEEPEEDEE_LIB_SENDFILE_HH

class Sendfile 
{
  int _read_fd;
  int _write_fd;

  struct Buf
  {
    char buffer[8192];
    unsigned len;
    unsigned pos;

    Buf():len(0),pos(0)
    {
    }
  };
  Buf*_buf;
public:
  Sendfile(int wfd=-1,int rfd=-1):
    _read_fd(rfd),
    _write_fd(wfd),
    _buf(0)
  {
  }

  void
  set_read_fd(int rfd)
  {
    _read_fd =rfd;
  }
  int
  get_read_fd()const
  {
    return _read_fd;
  }
  
  void
  set_write_fd(int rfd)
  {
    _write_fd =rfd;
  }
  int
  get_write_fd()const
  {
    return _write_fd;
  }

  bool
  data_buffered()
  {
    if(!_buf)
      return false;
    return (_buf->pos < _buf->len);
  }
  
  // returns true if finished
  // perform one io system call only
  bool
  io();

  void
  close();

  ~Sendfile()
  {
    close();
  }
};

#endif

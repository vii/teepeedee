#ifndef _TEEPEEDEE_FTPDATALISTENER_HH
#define _TEEPEEDEE_FTPDATALISTENER_HH

#include <iocontextlistener.hh>

class IOContext;
class FTPDataListener : public IOContextListener
{
  typedef IOContextListener super;
  
  IOContext*_data;
  bool _finished;
  bool finished()const
  {
    return _finished;
  }
  void
  release_data()
  {
    _data = 0;
    _finished = true;
  }
protected:
  void // return true if want to read more
  read_in(Stream&stream,size_t max)
  {
    if(finished()){
      delete_this();
      return;
    }
    if(!has_data())return;
    super::read_in(stream,max);
  }
public:
  FTPDataListener():_data(0),_finished(false)
  {
  }

  void
  set_data(IOContext*fdc)
    ;

  bool
  has_data()const
  {
    return _data != 0;
  }

  void
  free()
  {
    if(has_data()){
      delete _data;
    }
    release_data();
  }

  IOContext*
  new_iocontext()
    ;

  
  bool
  want_read(Stream&stream)
  {
    if(finished()){
      delete_this();
      return false;
    }
    if(!has_data()){
      return false;
    }
    return super::want_read(stream);
  }

  ~FTPDataListener()
  {
    free();
  }
};

#endif

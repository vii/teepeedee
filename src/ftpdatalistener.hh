#ifndef _TEEPEEDEE_FTPDATALISTENER_HH
#define _TEEPEEDEE_FTPDATALISTENER_HH

#include <listener.hh>

class IOContext;
class FTPDataListener : public Listener
{
  class IOContext*_data;
public:
  FTPDataListener():_data(0)
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
  
  events_t get_events()
    ;
  
  bool
  new_connection(int newfd,XferTable&xt)
    ;  
};

#endif

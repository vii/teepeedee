#ifndef _TEEPEEDEE_LIB_XFERTABLE_HH
#define _TEEPEEDEE_LIB_XFERTABLE_HH

#include <list>
#include <map>
class IOContext;

typedef std::list<IOContext*> XferTable_super;
class XferTable : private XferTable_super
{
  typedef std::map<int,IOContext*> _fd_to_xfer_type;
  _fd_to_xfer_type _fd_to_xfer;
public:
  void
  poll();
  void
  add(IOContext*ic)
  {
    push_back(ic);
  }
  void
  del(IOContext*ic)
    ;
  
  bool
  empty()const
  {
    return XferTable_super::empty();
  }
  
private:
  bool
  do_poll();

};
#endif

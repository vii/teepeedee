#ifndef _TEEPEEDEE_LIB_XFERTABLE_HH
#define _TEEPEEDEE_LIB_XFERTABLE_HH

#include <list>
#include <map>
#include <ctime>
#include <sys/types.h>

class Stream;
class StreamFD;

// Stream's based on filedescriptors are treated differently in order
// to do poll on them.
class XferTable : private std::list<Stream*>
{
  typedef std::list<Stream*> super;
  typedef std::map<int,StreamFD*> _fd_to_xfer_type;
  _fd_to_xfer_type _fd_to_xfer;

  size_t
  fill_fds(struct pollfd*fds, size_t nfds, bool& has_timeout)
  ;

  bool
  check_timeouts(std::time_t now, std::time_t& last_sweep,bool&has_timeout)
    ;

  bool reap();
  
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

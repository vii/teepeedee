#ifndef _TEEPEEDEE_LIB_STREAMTABLE_HH
#define _TEEPEEDEE_LIB_STREAMTABLE_HH

#include <list>
#include <vector>
#include <map>
#include <set>

#include <signal.h>

#include "streamcontainer.hh"
class Stream;
class StreamFD;
class IOContext;

class StreamTable:public StreamContainer
{
  typedef std::list<Stream*> _streams_type;
  _streams_type _streams;
  typedef std::list<Stream*> _nonfd_streams_type;
  _nonfd_streams_type _nonfd_streams;
  typedef std::map<int,Stream*> _fd_to_stream_type;
  _fd_to_stream_type _fd_to_stream;

  // note that an added stream might be deleted
  typedef std::set<Stream*> _deleted_streams_type;
  _deleted_streams_type _deleted_streams;
  typedef _deleted_streams_type _added_streams_type;
  _deleted_streams_type _added_streams;

  sig_atomic_t _async_notify;
  void
  free()
    ;
  
  void
  sow_and_reap()
    ;

  void
  remove(Stream*s)
  {
    _deleted_streams.insert(s);
  }

  void
  remove(IOContext*ioc)
    ;
  

  
  void
  do_remove(Stream*s)
    ;
  void
  do_add(Stream*s)
    ;

  bool
  do_events(Stream*s)
    ;

  bool
  tables_modified()const
  {
    return !_added_streams.empty() || !_deleted_streams.empty();
  }
  
  StreamTable(const StreamTable&)
  {
  }
public:
  
  
  StreamTable():_async_notify(0)
  {
  }

  void
  async_notify_stop()
  {
    _async_notify = 1;
  }
  
  void
  add(Stream*s)
  {
    _added_streams.insert(s);
  }

  void
  poll(int const*masked_signals);

  ~StreamTable()
  {
    free();
  }
  bool
  empty()const
  {
    return _added_streams.empty()&&_streams.empty();
  }
  
};

#endif

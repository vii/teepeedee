#ifndef _TEEPEEDEE_LIB_STREAMTABLE_HH
#define _TEEPEEDEE_LIB_STREAMTABLE_HH

#include <list>
#include <vector>
#include <map>
#include <set>

class Stream;
class StreamFD;
class IOContext;

class StreamTable
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
  
  void
  free()
    ;
  
  void
  sow_and_reap()
  {
    // do not change the order!! as added streams might also be deleted
    for(_added_streams_type::iterator i=_added_streams.begin();
	i!=_added_streams.end();++i)
      do_add(*i);
    _added_streams.clear();
    
    for(_deleted_streams_type::iterator i=_deleted_streams.begin();
	i!=_deleted_streams.end();++i)
      do_remove(*i);
    _deleted_streams.clear();
  }
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
  StreamTable()
  {
  }
  
  void
  add(Stream*s)
  {
    _added_streams.insert(s);
  }
  void
  remove(Stream*s)
  {
    _deleted_streams.insert(s);
  }
  void
  remove(IOContext*ioc)
    ;
  
  void
  poll();

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

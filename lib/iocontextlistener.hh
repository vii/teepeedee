#ifndef _TEEPEEDEE_LIB_IOCONTEXTLISTENER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTLISTENER_HH

#include <string>
#include "iocontext.hh"
#include "streamtable.hh"

class IOContextListener:public IOContext
{
  StreamTable*_table;
  
protected:
  void
  read_in(Stream&stream,size_t max)
  {
    while(Stream*s=do_accept(stream)){
      Stream*t;
      try {
	t= new_stream(s,*_table);
      } catch (...){
	delete s;
	throw;
      }
      try{
	t->consumer(new_iocontext());
	(*_table).add(t);
      } catch (...){
	delete t;
	throw;
      }
    }
  }
public:
  IOContextListener():_table(0)
  {
  }

  void
  stream_container(StreamTable*table)
  {
    _table=table;
  }
  StreamTable&
  stream_container()
  {
    return *_table;
  }

  bool
  want_write(Stream&stream)
  {
    return false;
  }
  
  bool
  want_read(Stream&stream)
  {
    return true;
  }
  
  virtual
  IOContext*
  new_iocontext()
    =0;

  virtual
  Stream*
  new_stream(Stream*source,StreamTable&table)
  {
    return source;
  }
  
  std::string
  desc()const
  {
    return "listener";
  }

};
#endif

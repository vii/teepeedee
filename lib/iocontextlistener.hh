#ifndef _TEEPEEDEE_LIB_IOCONTEXTLISTENER_HH
#define _TEEPEEDEE_LIB_IOCONTEXTLISTENER_HH

#include <string>
#include "iocontext.hh"
#include "streamcontainer.hh"

class IOContextListener:public virtual IOContext
{
  StreamContainer*_table;
  
protected:
  void
  read_in(Stream&stream,size_t max)
  {
    while(Stream*s=do_accept(stream)){
      Stream*t;
      try {
	t= new_stream(s,stream_container());
      } catch (...){
	delete s;
	throw;
      }
      try{
	IOContext*ioc = new_iocontext();
	if(!ioc){
	  delete t;
	  return;
	}
	t->consumer(ioc);
	stream_container().add(t);
      } catch (...) {
	delete t;
	throw;
      }
      t->consumer()->remote_stream(*s);
    }
  }
public:
  IOContextListener():_table(0)
  {
  }

  void
  stream_container(StreamContainer*table)
  {
    _table=table;
  }
  StreamContainer&
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
  new_stream(Stream*source,StreamContainer&table)
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

#ifndef _TEEPEEDEE_LIB_IOCONTEXTLIST_HH
#define _TEEPEEDEE_LIB_IOCONTEXTLIST_HH

#include <string>

#include "iocontextwriter.hh"
#include "iocontextcontrolled.hh"
#include "filelister.hh"
#include "path.hh"

class IOContextList : public IOContextWriter, public IOContextControlled
{
  FileLister*_lister;
  std::string _buf;
  
  void
  free()
  {
    if(_lister){
      delete _lister;
      _lister = 0;
    }
  }
  void next_entry()
  {
    if(no_entries_left())
      return;
    prepare_entry();
    skip_entry();
  }

protected:
  void
  skip_entry()
  {
    if(no_entries_left())
      return;
    
    _lister->next();
  }
  bool no_entries_left()const
  {
    return !_lister || _lister->last();
  }
  Path
  virtual_path()const
  {
    return _lister->virtual_path();
  }
  
  std::string
  entry_name()const
  {
    return _lister->name();
  }
  
  void
  finished_writing(class XferTable&xt)
  {
    if(no_entries_left()) {
      finished_listing(xt);
      return;
    }
    next_entry();
  }
  virtual
  void
  finished_listing(class XferTable&xt)
  {
    successful();
    hangup(xt);
  }

  virtual
  void
  prepare_entry()
    =0;

  void
  set_entry(const std::string&s)
  {
    _buf = s;
    set_write_buf(_buf.c_str(),_buf.length());
  }
  
public:
  
  IOContextList(IOController*ioc,FileLister*l):
    IOContextControlled(ioc),
    _lister(l)
  {
  }

  std::string desc()const
  {
    return "directory listing";
  }
  
  ~IOContextList()
  {
    free();
  }
};

  

#endif

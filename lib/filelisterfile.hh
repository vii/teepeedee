#ifndef _TEEPEEDEE_LIB_FILELISTERFILE_HH
#define _TEEPEEDEE_LIB_FILELISTERFILE_HH

#include <string>
#include "filelister.hh"
#include "path.hh"

class FileListerFile:public FileLister
{
  Path _path;
public:
  FileListerFile(const Path&p):_path(p)
  {
    
  }
  bool
  last()const
  {
    return _path.empty();
  }
  virtual void
  next()
  {
    _path.clear();
  }
  
  virtual
  std::string
  name()
    const
  {
    return _path.get_last();
  }

  virtual
  Path
  virtual_path()
    const
  {
    return _path;
  }

};

#endif

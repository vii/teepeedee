#ifndef _TEEPEEDEE_LIB_FILELISTERNONE_HH
#define _TEEPEEDEE_LIB_FILELISTERNONE_HH

#include <string>
#include "filelister.hh"
#include "path.hh"

class FileListerNone:public FileLister
{
public:
  virtual
  bool
  last()const
  {
    return true;
  }
  
  virtual void
  next()
  {
    return;
  }
  
  virtual
  std::string
  name()
    const
  {
    return std::string();
  }

  virtual
  Path
  virtual_path()
    const
  {
    return Path();
  }

};

#endif

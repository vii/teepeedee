#ifndef _TEEPEEDEE_LIB_FILELISTER_HH
#define _TEEPEEDEE_LIB_FILELISTER_HH

#include <string>
class Path;

class FileLister
{
public:
  virtual
  bool
  last()const
    =0;
  
  virtual void
  next()
    =0;
  
  virtual
  std::string
  name()
    const=0;

  virtual
  Path
  virtual_path()
    const=0;
  
  virtual ~FileLister()
  {
  }  
};

#endif

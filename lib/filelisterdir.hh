#ifndef _TEEPEEDEE_LIB_FILELISTERDIR_HH
#define _TEEPEEDEE_LIB_FILELISTERDIR_HH

#include <string>
#include "path.hh"
#include "filelister.hh"
#include "dircontainer.hh"

class FileListerDir : public FileLister,
		      private DirContainer
{
  Path _vpath;
  DirContainer::iterator _entry;
  
public:
  FileListerDir(const std::string& dir,const Path&vpath):
    DirContainer(dir),
    _vpath(vpath),
    _entry(begin())
  {
  }
  int
  get_fd()
  {
    return _entry.get_fd();
  }
  void
  next()
  {
    ++_entry;
  }

  bool
  last()const
  {
    return _entry == end();
  }
  
  std::string
  name()
    const
  {
    return *_entry;
  }

  Path
  virtual_path()
    const
  {
    Path p(_vpath);
    p.push_back(name());
    return p;
  }
};

#endif

#include <errno.h>

#include "dircontainer.hh"

#ifndef HAVE_READDIR_R
static
int readdir_r(DIR *d, struct dirent *e,
		struct dirent **r)
{
  errno = 0; // ffs is this stupid
  *r = readdir(d);
  if(!*r){
    if(errno)
      return -1;
    return 0;
  }
  return 0;
}
#endif

void
DirContainer::iterator::read_next()
{
  if(!_stream)
    return;

  struct dirent*e;
  if(readdir_r(_stream,&_entry,&e)){
    warn("readdir_r");
    close();
    return;
  }
  if(!e){
    close();
    return;
  }
  if(e!=&_entry)
    memcpy(&_entry,e,sizeof _entry);
      
  if(_entry.d_name[0] == '.' && (!_entry.d_name[1] || (_entry.d_name[1] == '.' && !_entry.d_name[2])))
    read_next();
  
  _entry.d_name[sizeof(_entry.d_name)-1]=0;
}

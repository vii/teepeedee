#ifndef _TEEPEEDEE_LIB_DIRCONTAINER_HH
#define _TEEPEEDEE_LIB_DIRCONTAINER_HH

#include <string>

#include <dirent.h>
#include <err.h>
#include <string.h>

class DirContainer
{
  std::string _dir;
  
public:
  DirContainer(const std::string& d):_dir(d)
  {
  };

  std::string name()const
  {
    return _dir;
  }
  
  
  class iterator 
  {
    DIR*_stream;
    struct dirent _entry;

  protected:

    friend class DirContainer;
    
    iterator(DIR*s):_stream(s)
    {
      read_next();
    }
    
    void
    close() 
    {
      if(_stream && closedir(_stream))
	warn("closedir");
      _stream=0;
    }

    void
    read_next()
      ;

  public:

    bool operator!()const
    {
      return !_stream;
    }

    int
    get_fd()
    {
      if(!_stream)return -1;
      return dirfd(_stream);
    }
    
    ~iterator()
    {
      close();
    }
    std::string
    get_name()const
    {
      if(!_stream){
	return std::string();
      }
      
      return std::string(_entry.d_name);
    }
        
    std::string operator*()const
    {
      return get_name();
    }

    iterator&operator++()
    {
      read_next();
      return *this;
    }

    bool operator == (const iterator&i)const
    {
      if(!_stream || !i._stream)return (!_stream && !i._stream);
      return(*(*this) == *i);
    }
    bool operator != (const iterator&i)const
    {
      return !(*this == i);
    }
  };

  iterator begin()const
  {
    DIR*d=opendir(_dir.c_str());
    return iterator(d);
  }

  iterator end()const
  {
    return iterator(0);
  }
};


#endif

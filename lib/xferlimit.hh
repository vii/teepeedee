#ifndef _TEEPEEDEE_LIB_XFERLIMIT_HH
#define _TEEPEEDEE_LIB_XFERLIMIT_HH

#include <string>
#include <iostream>

#include <sys/types.h>

#include "xferlimitexception.hh"
#include "time.hh"

class XferLimit{
  off_t _total;
  Time _start;
  std::string _remotename;
  std::string _service;
  std::string _username;
  std::string _resource;
public:
  class Direction
  {
  public:
    enum type {
      upload,download
    };
    static
    std::string
    to_string(type d)
    {
      switch(d){
      default:return (char*)0;
      case upload:return "upload";
      case download:return "download";
      }
    };
    static
    char
    to_char(type d)
    {
      switch(d){
      default:return ' ';
      case upload:return 'i';
      case download:return 'o';
      }
    };
    
  };
private:
  Direction::type _dir;
protected:
  virtual
  bool // true if allowed to xfer that many bytes
  book_xfer(off_t bytes,Direction::type dir)
  {
    return true;
  };
public:
  XferLimit(Direction::type dir,
	    const std::string& remotename,
	    const std::string& service,
	    const std::string&resource,
	    const std::string&username):
    _total(0),
    _remotename(remotename),
    _service(service),
    _username(username),
    _resource(resource),
    _dir(dir)
  {
  }
  
  void
  direction(Direction::type d)
  {
    _dir = d;
  }
  void
  remotename(const std::string&rm)
  {
    _remotename = rm;
  }
  void
  service(const std::string&rm)
  {
    _service = rm;
  }
  void
  username(const std::string&s)
  {
    _username = s;
  }
  void
  resource(const std::string&s)
  {
    _resource = s;
  }

  void
  xfer(off_t bytes)
  {
    if(!book_xfer(bytes,_dir))
      throw XferLimitException();
    _total += bytes;
  }
  
  virtual ~XferLimit()
  {
    //xferlog format as described http://www.id.ethz.ch/Dienste/Lists/Archiv/sun-unix/0041.html
    Time end;
    end -= _start;
    std::cout << _start << ' '
	      <<  end.seconds() << ' '
	      << _remotename << ' '
	      << _total << ' '
	      << _resource << ' '
	      << "b _ "
	      << Direction::to_char(_dir) << ' '
	      << "g "
	      << _username << ' '
	      << _service << ' '
	      << "0 *" << std::endl;
  }    
};


#endif

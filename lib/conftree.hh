#ifndef _TEEPEEDEE_LIB_CONFTREE_HH
#define _TEEPEEDEE_LIB_CONFTREE_HH

#include <string>
#include <fstream>
#include <ctime>
#include <sstream>

#include <err.h>
#include <sys/types.h>
#include <arpa/inet.h> // for uint32_t

#include "confexception.hh"

class IOContext;

class ConfTree
{
  std::string _name;

  void
  open(std::ifstream&i,const std::string&n)const
    ;
  void
  open(std::ofstream&i,const std::string&n)const
    ;

  std::string
  make_path(const std::string&name)const
    ;

   void
  get_seconds(const std::string&name,std::time_t&out)const
    ;

  class Lock
  {
  public:
    Lock(ConfTree&tree,const std::string&name)
    {//XXX TODO
    }
    ~Lock()
    {
    }
  };
  friend class Lock;
  
public:

  // NAME is currently the name of a directory in the fs
  ConfTree(const std::string&name=std::string()):_name(name)
  {
  }
  void
  set_name(const std::string&name)
  {
    _name = name;
  }

  template<class T>
  void
  get_if_exists(const std::string&name,T&val)
  {
    if(!exists(name))
      return;
    get(name,val);
  }

  template<class T>
  void
  get_atomically(const std::string&name,T&val)
  {
    Lock up(*this,name);
    get(name,val);
  }
  
  template<class T>
  void
  get_atomically_if_exists(const std::string&name,T&val)
  {
    if(!exists(name))
      return;
    Lock up(*this,name);
    get(name,val);
  }
  
  bool
  exists(const std::string&name)
    const;

  void
  get(const std::string&name,ConfTree&tree)
    const;

  void
  get(const std::string&name,std::string&out)const
    ;
  
  void
  get(const std::string&name,bool&out)const
  {
    out = exists(name);
  }
  
  void
  get_line(const std::string&name,std::string&val)const
    ;

  void
  get_link(const std::string&name,std::string&val)const
    ;
  void
  get_fsname(const std::string&name,std::string&val)const
    ;

  template<class T>
  void
  get(const std::string&n,T&i)const
  {
    std::ifstream in;
    open(in,n);
    in>>i;
    if(!in)
      throw ConfException(n,"error reading value");
  }
  template<class T>
  void
  put(const std::string&name,const T& i)
  {
    std::ofstream out;
    open(out,name);
    out<<i;
    if(!out.good())
      throw ConfException(name,"error writing value");
  }

  template<class T>
  bool
  book_increment(const std::string&value,T increment)
  {
    return book_increment(value,value + "_max",increment);
  }
  
  template<class T>
  bool//return true if increment +value is below max or max is 0
  book_increment(const std::string&value,const std::string&maximum,T increment)
  {
    T max = 0;
    get_if_exists(maximum,max);
    return book_increment(value,max,increment);
  }
  
  template<class T>
  bool//return true if increment + value is below max or max is 0
  book_increment(const std::string&value,T max,T increment)
  {
    ConfTree::Lock up(*this,value);
    T current = 0;
    get_if_exists(value,current);
    if(!max || current + increment < max){
      if(increment)
	put(value,current+increment);
      return true;
    }
    return false;
  }

  template<class T>
  void
  book_decrement(const std::string&value, T decrement)
  {
    ConfTree::Lock up(*this,value);
    T current = 0;
    get_if_exists(value,current);
    if(decrement > current && (current-decrement)>0) { //unsigned
      current = 0;
      warnx("counter %s underflowed when being decremented",value.c_str());
    }
    else {
      current = current - decrement;
    }
    put(value,current); 
  }
  
  void
  get_ipv4_addr(const std::string&name,uint32_t&inaddr)const;

  void
  get_timeout(const std::string&name,IOContext&ioc)const;
};


#endif

#ifndef _TEEPEEDEE_LIB_CONFDB_HH
#define _TEEPEEDEE_LIB_CONFDB_HH

#include <string>
#include <map>
#include <sstream>

#include <sys/types.h>

#include "dircontainer.hh"
#include "confexception.hh"

class ConfDB
{
  std::string _base;
 public:
  typedef off_t Numeric; // unsigned
  typedef std::string Key;
 private:

  
  class Value
  {
    std::string _contents;
    bool _changed;
  public:
    Value(const std::string&s=std::string(),bool c=false):
      _contents(s),_changed(c)
    {
    }
    void
    set(const std::string&s)
    {
      if(!_changed){
	if(_contents != s){
	  _changed = true;
	  _contents = s;
	}
      } else {
	_contents = s;
      }
    }
    const std::string&
    get()const
    {
      return _contents;
    }
    bool
    changed()const
    {
      return _changed;
    }
    void
    unchange()
    {
      _changed = false;
    }
  };
    
  typedef std::map<Key,Value> Hash;
  Hash _hash;


  std::string
  make_path(const Key&k)const
  {
    return join_keys(_base, k);
  }
  
 public:
  ConfDB(const std::string path):_base(path)
  {
  }
  
  template<class T>
  void
  get_subkeys(const Key&k,T&container)
  {
    DirContainer d(make_path(k));
    for(DirContainer::iterator i = d.begin();!!i;++i)
      container.push_back(*i);
  }

  void
  write_out()
    ;
  
  void sync()
  {
    write_out();
    _hash.erase(_hash.begin(),_hash.end());
  }

  //always true if the value +  amt <= max or max == 0
  bool
  increment(const Key&k,Numeric amt=1, Numeric max=0)
    ;
  bool
  decrement(const Key&k,Numeric amt=1, Numeric min=0)
    ;

  bool
  exists(const Key&k)
    ;

  // This is used by the OpenSSL wrapper, for finding where its
  // keyfiles are. It should only be used for things like this.
  std::string
  get_fsname(const Key&k)
  {
    return make_path(k);
  }
  
  std::string
  get(const Key&k)
    ;

  template<class T>
  void
  get_type(const Key&k,T&ret)
  {
    std::istringstream iss(get(k));
    iss >> ret;
    if(!iss)
      throw ConfException(k,"unable to correctly read value");
  }
  
  void
  put(const Key&k,const std::string&str)
    ;

  template<class T>
  void
  put_type(const Key&k,const T&ret)
  {
    std::ostringstream oss;
    oss << ret;
    if(!oss.good())
      throw ConfException(k,"unable to correctly write value");
    put(k,oss.str());
  }

  Key
  join_keys(const Key&base,const Key&key)const
  {
    return base + "/" + key;
  }
  
  ~ConfDB()
  {
    write_out();
  }
};

#endif

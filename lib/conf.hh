#ifndef _TEEPEEDEE_LIB_CONF_HH
#define _TEEPEEDEE_LIB_CONF_HH

#include <string>
#include <fstream>
#include <ctime>
#include <sstream>

#include <err.h>
#include <sys/types.h>
#include <arpa/inet.h> // for uint32_t

#include "confexception.hh"
#include "confdb.hh"

class IOContext;

class Conf
{
  ConfDB::Key _key;
  ConfDB mutable* _db;
  
  ConfDB&
  db()const
  {
    return *_db;
  }
  
  void
  get_seconds(const std::string&name,std::time_t&out)const
    ;

  ConfDB::Key
  db_key(const ConfDB::Key&k)const
  {
    return db().join_keys(_key,k);
  }
  
public:
  Conf():_db(0)
  {
  }
  
  Conf(ConfDB&db,ConfDB::Key k = ConfDB::Key()):
    _key(k),
    _db(&db)
  {
  }

  Conf(const Conf&c,const ConfDB::Key& k):
    _key(c._key),
    _db(c._db)
  {
    key(db_key(k));
  }
  
  void
  key(const ConfDB::Key&name)
  {
    _key = name;
  }

  bool
  exists(const ConfDB::Key&name)
    const
  {
    return db().exists(db_key(name));
  }

  template<class T>
  void
  get_atomically_if_exists(const ConfDB::Key&name,T&val)
  {
    return get_if_exists(name,val);
  }

  
  template<class T>
  void
  get_if_exists(const ConfDB::Key&name,T&val)
  {
    if(!exists(name))
      return;
    get(name,val);
  }

  void
  get(const ConfDB::Key&name,std::string&out)const
  {
    out = db().get(db_key(name));
  }
  void
  get(const ConfDB::Key&name,bool&out)const
  {
    out = exists(name);
  }
  
  void
  get_line(const ConfDB::Key&name,std::string&val)const
  {
    get(name,val);
    val = val.substr(0,val.find('\n'));
  }

  void
  get_fsname(const ConfDB::Key&name,std::string&val)const
  {
    val = db().get_fsname(db_key(name));
  }

  template<class T>
  void
  get(const ConfDB::Key&n,T&i)const
  {
    db().get_type(db_key(n),i);
  }
  template<class T>
  void
  put(const ConfDB::Key&name,const T& i)
  {
    db().put_type(db_key(name),i);
  }
  bool
  book_increment(const ConfDB::Key&value,ConfDB::Numeric increment=1)
  {
    return book_increment(value,value + "_max",increment);
  }
  bool//return true if increment +value is below max or max is 0
  book_increment(const ConfDB::Key&value,const std::string&maximum,ConfDB::Numeric increment=1)
  {
    ConfDB::Numeric max = 0;
    get_if_exists(maximum,max);
    return book_increment(value,max,increment);
  }
  bool//return true if increment + value is below max or max is 0
  book_increment(const ConfDB::Key&value,ConfDB::Numeric max,ConfDB::Numeric increment=1)
  {
    return db().increment(db_key(value),increment,max);
  }

  void
  book_decrement(const ConfDB::Key&value, ConfDB::Numeric decrement=1)
  {
    if(!db().decrement(db_key(value),decrement))
      warnx("counter %s underflowed when being decremented",value.c_str());
  }
  
  void
  get_ipv4_addr(const ConfDB::Key&name,uint32_t&inaddr)const;

  void
  get_timeout(const ConfDB::Key&name,IOContext&ioc)const;

  template<class T>
  void
  get_subkeys(T&container)const
  {
    db().get_subkeys(_key,container);
  }

  std::string
  get_name()const
  {
    return _key;
  }

};


#endif

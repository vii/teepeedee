#ifndef _TEEPEEDEE_LIB_CONFTREE_HH
#define _TEEPEEDEE_LIB_CONFTREE_HH

#include <string>
#include <fstream>
#include <ctime>

#include <sys/types.h>
#include <arpa/inet.h> // for uint32_t

class IOContext;

class ConfTree
{
  std::string _name;

  void
  open(std::ifstream&i,const std::string&n)const
    ;

  std::string
  make_path(const std::string&name)const
    ;
  
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

  bool
  exists(const std::string&name)
    const;

  void
  get(const std::string&name,ConfTree&tree)
    const;
  
  
  void
  get(const std::string&name,std::string&val)
    const;

  void
  get(const std::string&name,bool&out)const
  {
    out = exists(name);
  }
  void
  get(const std::string&name,std::time_t&out)const
    ;
  
  void
  get_line(const std::string&name,std::string&val)const
    ;

  void
  get_link(const std::string&name,std::string&val)const
    ;
  void
  get_fsname(const std::string&name,std::string&val)const
    ;
  
  void
  get(const std::string&name,int&i)const;

  void
  get_ipv4_addr(const std::string&name,uint32_t&inaddr)const;

  void
  get_timeout(const std::string&name,IOContext&ioc)const;
};


#endif

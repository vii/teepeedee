#ifndef _TEEPEEDEE_SRC_PATH_HH
#define _TEEPEEDEE_SRC_PATH_HH

#include <list>
#include <string>

class Path : public std::list<std::string>
{
  
public:
  Path()
  {
  }
  Path(const std::string&dir)
  {
    append(dir);
  }
  Path(const std::string&dir,const std::string&then)
  {
    append(dir);
    chdir(then);
  }
  Path(const Path&dir,const std::string&then)
  {
    *this = dir;
    chdir(then);
  }
  
  void
  clear()
  {
    erase(begin(),end());
  }

  std::string
  str()const
  {
    std::string ret;
    if(empty())return ret;

    const_iterator i = begin();
    ret += *i;
    for(++i;i!=end();++i)
      ret += std::string("/") + *i;

    return ret;
  }
//     operator std::string()const
//   {
//     return str();
//   }

  void
  chdir(const std::string&elem)
  {
    if(elem.empty())
      return;
    if(elem[0] == '/')
      clear();
    append(elem);
  }
  
  void
  append(const std::string&name)
    ;
  void
  append_one(const std::string&name)
    ;

  std::string
  get_last()const
  {
    if(empty())return std::string();
    return *rbegin();
  }
};

#endif

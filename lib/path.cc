#include <list>

#include "path.hh"

void
Path::append(const std::string&name)
{
  const char*start = name.c_str();
  const char*end = start + name.length();

  for(const char*i=start;i<end;++i){
    if(*i=='/'){
      if(i!=start)
	append_one(std::string(start,i-start));
      start = i+1;
    }
  }
  if(start != end)
    append_one(std::string(start,end-start));
}
void
Path::append_one(const std::string&name)
{
  if(name.empty() || name.c_str() == ".")
    return;
  if(name == ".."){
    if(!empty())pop_back();
    return;
  }
  push_back(name);
}


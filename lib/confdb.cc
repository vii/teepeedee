#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <exception>
#include "confdb.hh"

class
ConfDBIOException : public std::exception 
{
  std::string _str;
 public:
  ConfDBIOException(const std::string&s)
  {
    _str = s;
  }
  const char*what()const throw()
  {
    return _str.c_str();
  }
  ~ConfDBIOException() throw()
  {
  }
};

bool
ConfDB::exists(const Key&k)
{
  if(_hash.find(k)!=_hash.end())return true;
  
  std::string file = make_path(k);
  struct stat buf;

  int ret = stat(file.c_str(),&buf);
  return ret == 0;
}


bool
ConfDB::increment(const Key&k,Numeric amt, Numeric max)
{
  Numeric val=0;
  if(exists(k))
    get_type(k,val);
  val += amt;
  if(max && val > max)
    return false;
  put_type(k,val);
  return true;
}


bool
ConfDB::decrement(const Key&k,Numeric amt, Numeric min)
{
  Numeric val = min;
  if(exists(k))
    get_type(k,val);
  if(min + amt > val)
    return false;
  val -= amt;
  put_type(k,val);
  return true;
}

std::string
ConfDB::get(const Key&k)
{
  Hash::iterator i = _hash.find(k);
  if(i != _hash.end())
    return (*i).second.get();

  std::ifstream file(make_path(k).c_str());
  if(!file.good()){
    throw ConfException(k,"unable to open file for reading: " + make_path(k));
  }
  
  std::string val;
  for(char c;file.good();val+=c){
    c = file.get();
  }
  
  _hash[k] = Value(val);
  return val;
}
    
void
ConfDB::put(const Key&k,const std::string&str)
{
  _hash[k].set(str);
}

void
ConfDB::write_out()
{
  std::string badfiles;
  for(Hash::iterator i = _hash.begin();i!=_hash.end();++i)
    if(i->second.changed()){
      std::ofstream file(make_path(i->first).c_str());
      if(!file.good()){
	badfiles +=  make_path(i->first) + " ";
	continue;
      }
      file << i->second.get();
      file.close();
      if(!file.good()){
	badfiles +=  make_path(i->first) + " ";
	continue;
      }
      i->second.unchange();
    }
  if(!badfiles.empty())
    throw ConfDBIOException("unable to sync files: " + badfiles);
}



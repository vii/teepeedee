#include <fstream>
#include <iostream>
#include <string>

#include <cstring>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "conftree.hh"
#include "confexception.hh"
#include "iocontext.hh"

void
ConfTree::get_fsname(const std::string&name,std::string&val)const
{
  if(!exists(name))
    throw ConfException(name,"non-existant filesystem name");
  val = make_path(name);
}

void
ConfTree::get_link(const std::string&name,std::string&val)const
{
  char*buffer = 0;
  std::string path = make_path(name);
  for(int buffer_size = 100;buffer_size < 10000;buffer_size *= 2){
    buffer = (char*)realloc(buffer,buffer_size+1);
    if(!buffer)
      throw ConfException(name,"out of memory reading link");
    
    int len = readlink(path.c_str(), buffer, buffer_size);
    if(len == -1){
      free(buffer);
      throw ConfException(name,"not a link");
    }
    if(len != buffer_size){
      buffer[len] = 0;
      val = buffer;
      free(buffer);
      return;
    }
  }

  free(buffer);
  throw ConfException(name,"link ridiculously long");
}

std::string
ConfTree::make_path(const std::string&name)const
{
  return _name + '/' + name;
}

void
ConfTree::get(const std::string&name,std::string&val)const
{
  std::ifstream in;
  open(in,name);

  val = std::string();
  while(in.good())
    val += in.get();
}

void
ConfTree::open(class std::ifstream&i,const std::string&n)const
{
  i.open(make_path(n).c_str(), std::ifstream::in);
  if(!i.good())
    throw ConfException(n,"unable to read file");
}

void
ConfTree::get_line(const std::string&name,std::string&val)const
{
  std::ifstream in;

  open(in,name);
  
  std::getline(in,val);

  if(in.bad())
    throw ConfException(name,"unable to read line from file");
}


void
ConfTree::get(const std::string&n,int&i)const
{
  std::string v;
  get(n,v);

  if(v.empty())
    throw ConfException(n,"empty string: not a valid integer");

  char *s;
  i = strtol(v.c_str(),&s,0);
  
  if(s == v.c_str())
    throw ConfException(n,"not a valid integer");
}

void
ConfTree::get_ipv4_addr(const std::string&n,uint32_t&inaddr)const
{
  std::string v;
  get_line(n,v);
  if(inet_pton(AF_INET,v.c_str(),&inaddr)<=0)
    throw ConfException(n,"not an IPv4 address");
}

bool
ConfTree::exists(const std::string&name)const
{
  std::string file = make_path(name);
  struct stat buf;

  int ret = stat(file.c_str(),&buf);
  return ret == 0;
}

void
ConfTree::get(const std::string&name,ConfTree&tree)const
{
  if(!exists(name))
    throw ConfException(name,"tree does not exist");
  tree.set_name(make_path(name));
}

void
ConfTree::get(const std::string&name,std::time_t&out)const
{
  std::string v;
  get(name,v);

  if(v.empty())
    throw ConfException(name,"empty string: not a valid time_t");

  char *s;
  out = strtoul(v.c_str(),&s,0);
  
  if(s == v.c_str())
    throw ConfException(name,"not a valid time_T");
}

void
ConfTree::get_timeout(const std::string&name,IOContext&ioc)const
{
  if(!exists(name)){
    ioc.set_timeout_interval(0);
    return;
  }
  time_t t;
  get(name,t);
  ioc.set_timeout_interval(t);
}


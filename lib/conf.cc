#include <fstream>
#include <iostream>
#include <string>

#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "conf.hh"
#include "confexception.hh"
#include "iocontext.hh"

void
Conf::get_ipv4_addr(const ConfDB::Key&n,uint32_t&inaddr)const
{
  std::string v;
  get_line(n,v);
  if(inet_pton(AF_INET,v.c_str(),&inaddr)<=0)
    throw ConfException(n,"not an IPv4 address");
}

void
Conf::get_seconds(const ConfDB::Key&name,std::time_t&out)const
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
Conf::get_timeout(const ConfDB::Key&name,IOContext&ioc)const
{
  if(!exists(name)){
    ioc.set_timeout_interval(0);
    return;
  }
  time_t t;
  get_seconds(name,t);
  ioc.set_timeout_interval(t);
}

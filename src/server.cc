#include <sys/types.h>
#include <netinet/in.h>
#include <stream.hh>
#include "server.hh"

Stream*
Server::read_config()
{
  uint32_t inaddr = INADDR_ANY;
  int port;

  if(_conf.exists("bind_addr"))
    _conf.get_ipv4_addr("bind_addr",inaddr);
  _conf.get("bind_port",port);

  sockaddr_in name;
  memset(&name,0,sizeof name); // for NetBSD 1.6
  name.sin_family = AF_INET;
  name.sin_addr.s_addr = inaddr;
  name.sin_port = htons(port);

  Stream* s = Stream::listen((const sockaddr*)&name,sizeof name);
  s->consumer(this);
  return s;
}

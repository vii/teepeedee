#include <sys/types.h>
#include <netinet/in.h>
#include "server.hh"

void
Server::read_config(const std::string&confname)
{
  uint32_t inaddr = INADDR_ANY;
  int port;

  _conf.set_name(confname);

  if(_conf.exists("bind_addr"))
    _conf.get_ipv4_addr("bind_addr",inaddr);
  _conf.get("bind_port",port);

  bind_ipv4(inaddr,htons(port));
  listen();
}


#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "streamfd.hh"

static
std::string
sockaddr_to_str(const sockaddr_in&sai)
{
  std::ostringstream s;
  char buf[50];
  inet_ntop (AF_INET, &sai.sin_addr.s_addr, buf,sizeof buf);
  s << buf << ':' << ntohs(sai.sin_port);
  
  return s.str();
}

std::string
StreamFD::desc()const
{
  std::ostringstream s;
  s << "fd " << fd();
  try {
    std::string n = localname();
    s << " " << n;
  } catch (...){
  }
  return s.str();
}

std::string
StreamFD::localname()const
{
  sockaddr_in sai;
  Stream::getsockname(sai);
  return sockaddr_to_str(sai);
}

std::string
StreamFD::remotename()const
{
  sockaddr_in sai;
  Stream::getpeername(sai);
  return sockaddr_to_str(sai);
}


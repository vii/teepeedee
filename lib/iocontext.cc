#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "xfertable.hh"
#include "iocontext.hh"
#include "unixexception.hh"

std::string
IOContext::desc()const
{
    std::ostringstream s;
    s << "fd " << get_fd();
    try {
      std::string n = getsockname();
      s << " " << n;
    } catch (...){
    }
    return s.str();
}

void
IOContext::close()
{
  if(_stream) {
    delete _stream;
    _stream = 0;
  }
}

void
IOContext::become_ipv4_socket()
{
  close();
  
  int fd = socket (PF_INET, SOCK_STREAM, 0);
  if(fd == -1)
    throw UnixException("socket(PF_INET, SOCK_STREAM, 0)");
  stream(new Stream(fd));
}


void
IOContext::bind_ipv4(uint32_t in_addr,uint16_t port)
{
  if(get_fd() == -1)
    become_ipv4_socket();

  set_nonblock();
  
  struct sockaddr_in name;
  memset(&name,0,sizeof name); // for NetBSD 1.6
  name.sin_family = AF_INET;
  name.sin_port = port;
  name.sin_addr.s_addr = in_addr;

  if(::bind(get_fd(),(struct sockaddr*)&name,sizeof name)){
    UnixException ue("bind");
    throw ue;
  }
}


void
IOContext::bind_ipv4(uint32_t in_addr,uint16_t port_min,uint16_t port_max)
{
  int tries = port_max - port_min + 1;
  int port = (std::rand() % tries) + port_min;
  
  for(;tries >= 0;--tries){
    try{
      bind_ipv4(in_addr,htons(port));
      return;
    }
    catch(UnixException&ue){
      if(ue.get_errno() != EADDRINUSE || port == port_max)
	throw;
    }

    port ++;
    if(port > port_max)
      port = port_min;
  }
}


void
IOContext::discard_hangup(class XferTable&xt)
{
  stream()->shutdown();
  hangup(xt);
}

void
IOContext::hangup(class XferTable&xt)
{
  xt.del(this);
}

void
IOContext::set_nonblock(int fd)
{
  int flags = fcntl(fd,F_GETFL,0);
  if(flags==-1)
    throw UnixException("fcntl(F_GETFL)");
  flags |= O_NONBLOCK;
  if(fcntl(fd,F_SETFL,flags)==-1)
    throw UnixException("fcntl(F_SETFL)");
}

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


void
IOContext::getsockname(struct sockaddr_in&sai)const
{
  socklen_t len = sizeof sai;
  getsockname(&sai,&len);
}

void
IOContext::getsockname(void*addr,socklen_t*len)const
{
  if(::getsockname(get_fd(),(sockaddr*)addr,len))
    throw UnixException("getsockname"); //cannot ref desc as that uses getsockname
}

std::string
IOContext::getsockname()const
{
  sockaddr_in sai;
  getsockname(sai);
  return sockaddr_to_str(sai);
}

void
IOContext::getpeername(struct sockaddr_in&sai)const
{
  socklen_t len = sizeof sai;
  getpeername(&sai,&len);
}

void
IOContext::getpeername(void*addr,socklen_t*len)const
{
  if(::getpeername(get_fd(),(sockaddr*)addr,len))
    throw UnixException("getpeername on "+desc());
}

std::string
IOContext::getpeername()const
{
  sockaddr_in sai;
  getpeername(sai);
  return sockaddr_to_str(sai);
}

void
IOContext::setsockopt(int LEVEL, int OPTNAME, void
		      *OPTVAL, socklen_t OPTLEN)
{
  if(-1 == ::setsockopt(get_fd(),LEVEL,OPTNAME,OPTVAL,OPTLEN))
    throw UnixException("setsockopt");
}

void
IOContext::set_reuse_addr(bool on)
{
  int opt = on ? 1 : 0;
  setsockopt(SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
}


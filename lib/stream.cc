#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "streamfd.hh"
#include "stream.hh"
#include "iocontext.hh"

void
Stream::free()
{
  if(_consumer){
    if(_consumer->stream_hungup(*this))
      delete _consumer;
    _consumer=0;
  }
}


static
int
get_socket()
{
  int fd = socket (PF_INET, SOCK_STREAM, 0);
  if(fd == -1)
    throw UnixException("socket(PF_INET, SOCK_STREAM, 0)");
  return fd;
}


static
StreamFD*
bind_to(const sockaddr*addr,socklen_t len)
{
  StreamFD*stream=new StreamFD;
  try{
    stream->fd(get_socket());
    stream->set_reuse_addr();
    if(addr)
      if(::bind(stream->fd(),addr,len))
	throw UnixException("bind");
  
    return stream;
  } catch (...){
    delete stream;
    throw;
  }
}

Stream*
Stream::listen_ipv4_range(uint32_t inaddr,uint16_t port_min,uint16_t port_max)
{
  int tries = port_max - port_min + 1;
  int port = (std::rand() % tries) + port_min;
  sockaddr_in name;
  memset(&name,0,sizeof name); // for NetBSD 1.6
  name.sin_family = AF_INET;
  //  name.sin_addr.s_addr = in_addr;
  
  for(;tries >= 0;--tries){
    name.sin_port = htons(port);
    try{
      return listen((const sockaddr*)&name,sizeof name);
    }
    catch(const UnixException&ue){
      if(ue.get_errno() != EADDRINUSE || tries == 1)
	throw;
    }

    port ++;
    if(port > port_max)
      port = port_min;
  }
  *(int*)0=0;//can't get here
  return 0;
}

static
inline unsigned listen_queue_length()
{
  return 10;
}

Stream*
Stream::listen(const sockaddr*addr,socklen_t len)
{
  StreamFD*stream=bind_to(addr,len);
  try{
    if(::listen(stream->fd(),listen_queue_length())){
      throw UnixException("listen");
    }
  } catch (...){
    delete stream;
    throw;
  }
  return stream;
}

Stream*
Stream::connect(const sockaddr*addr,socklen_t len,const sockaddr*source_addr,socklen_t source_len)
{
  StreamFD*stream=bind_to(source_addr,source_len);
  try{
    int ret = ::connect(stream->fd(),addr,len);
    if(ret == -1){
      if(errno != EINPROGRESS)
	throw UnixException("connect to user port");
    }
  } catch (...){
    delete stream;
    throw;
  }
  return stream;
}

#include <sys/types.h>
#include <sys/poll.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "unixexception.hh"

#include "listener.hh"

static
int
listen_queue_length()
{
  return 16;
}


Listener::events_t
Listener::get_events()
{
  return POLLIN|POLLOUT;
}
  


void Listener::listen()
{
  if(::listen(get_fd(),listen_queue_length())){
    throw UnixException("listen");
  }
}

std::string
Listener::desc()const
{
  return std::string("listen ") + IOContext::desc();
}


bool
Listener::io(const struct pollfd&pfd,class XferTable&xt)
{
  int newfd = accept(get_fd(),0,0);
  
  if(newfd == -1){
    if(errno == EWOULDBLOCK)
      return false;
    
    warn("accept");
    return false;
  }

  set_nonblock(newfd);
  Stream*s;
  try{
    s=stream_factory(newfd);
  } catch (...){
    ::close(newfd);
    throw;
  }
  
  if(new_connection(s,xt))
    return true;
  return false;
}

  

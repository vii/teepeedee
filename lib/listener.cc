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
  return POLLIN;
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
  struct sockaddr_in sa;
  size_t sa_siz = sizeof sa;
  
  int newfd = accept(get_fd(),(sockaddr*)&sa,&sa_siz);
  
  if(newfd == -1){
    if(errno == EWOULDBLOCK)
      return false;
    
    warn("accept");
    return false;
  }

  set_nonblock(newfd);
  return new_connection(newfd,xt);
}

  

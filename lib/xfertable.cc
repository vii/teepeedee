#include <map>
#include <iostream>
#include <exception>
#include <ctime>

#include <sys/poll.h>
#include <err.h>
#include "unixexception.hh"
#include "xfertable.hh"
#include "iocontext.hh"

#define DBG(x) do { ; } while(0)

static const std::time_t sweep_interval = 30;

void
XferTable::del(IOContext*ic)
{
  remove(ic);
  if(ic->active()) {
    _fd_to_xfer_type::iterator i = _fd_to_xfer.find(ic->get_fd());
    if(i != _fd_to_xfer.end())
      _fd_to_xfer.erase(i);
  }
}

void
XferTable::poll()
{
  while(!do_poll());
}

bool
XferTable::do_poll()
{
  size_t nfds = size();
  bool rebuild_table = false;
  if(!nfds)return true;

  _fd_to_xfer.erase(_fd_to_xfer.begin(),_fd_to_xfer.end());
  
  struct pollfd *fds = new pollfd[nfds];
  
  try{
    
    bool do_sweep = false;
    
    rebuild_table = false;
    DBG(std::cout << "Watched file descriptors" << std::endl);
    unsigned j = 0;
    for(iterator i = begin(); i != end(); ++i){
      DBG(std::cout << (*i)->get_fd() << ":\t" << (*i)->desc() << ' ' << *i << std::endl);
      if(!(*i)->active())
	continue;

      if((*i)->get_timeout() != -1)
	do_sweep = true;

      fds[j].fd = (*i)->get_fd();
      fds[j].events = (*i)->get_events();
      fds[j].events |= POLLHUP;
      if(_fd_to_xfer.find(fds[j].fd)!=_fd_to_xfer.end())
	errx(1,"internal error: two active IOContexts with same fd!");
    
      _fd_to_xfer[fds[j].fd] = *i;
      if(++j > nfds)
	errx(1,"internal error in XferTable::poll, list changed under me!");
    }
    nfds = j;
    if(!nfds)
      goto out;

    std::time_t last_sweep = time(0);
    if(last_sweep == -1){
      throw UnixException("time(0)");
    }
  
    for(;;){
      int events = ::poll(fds,nfds,do_sweep ? sweep_interval : -1);

      if(events==-1){
	if(errno == EINTR)
	  continue;
	throw UnixException("poll");
      }

      std::time_t now = time(0);
      if(now == -1){
	throw UnixException("time(0)");
      }
    
      for(size_t i = 0;events && i<nfds;++i)
	if(fds[i].revents){
	  --events;
	  IOContext*x=_fd_to_xfer[fds[i].fd];
	  if(!x) {
	    rebuild_table = true;
	    continue;
	  }

	  try{
	    if(fds[i].revents & POLLNVAL)
	      warnx("%s gave a closed fd",x->desc().c_str());
	  
	    if(fds[i].revents & POLLHUP){
	      x->hangup(*this);
	      rebuild_table = true;
	    } else if(x->io_events(fds[i],now,*this))
	      rebuild_table = true;
	  }
	  catch(std::exception&e){
	    warnx("closing %s: %s",x->desc().c_str(),e.what());
	    rebuild_table = true;
	    x->hangup(*this);
	  }
	}

      if(now > last_sweep + sweep_interval){
	do_sweep = false;
	for(iterator prev=end(),k = begin();k!=end();++k){
	  if((*k)->check_timeout(now,*this)) {
	    rebuild_table = true;
	    if(prev == end())
	      k = begin();
	    else
	      k = prev;
	  } else {
	    if((*k)->has_timeout())
	      do_sweep = true;
	  }
	}
	last_sweep = now;
      }
    
      if(rebuild_table)
	break;
    }
  }catch(...){
    if(fds)
      delete[] fds;
    throw;
  }

 out:
  if(fds)
    delete[] fds;
  return false;
}

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

bool XferTable::reap() {
  if(_dead_ios.empty())return false;
  bool changed=false;
  for(_dead_ios_type::iterator i=_dead_ios.begin();
      i!=_dead_ios.end();){
    if(!(*i)->active() || !(*i)->get_stream_events()) {
      delete *i;
      i=erase(i);
      changed = true;
    } else
      ++i;
  }
  
  return changed;
}

static
std::time_t sweep_interval(){
  return 30;
}

void
XferTable::del(IOContext*ic)
{
  remove(ic);
  if(ic->active()) {
    _fd_to_xfer_type::iterator i = _fd_to_xfer.find(ic->get_fd());
    if(i != _fd_to_xfer.end())
      _fd_to_xfer.erase(i);
  }
  _dead_ios.push_back(ic);
}

void
XferTable::poll()
{
  while(!do_poll());
}

size_t
XferTable::fill_fds(struct pollfd*fds, size_t nfds, bool&has_timeout)
{
  _fd_to_xfer.erase(_fd_to_xfer.begin(),_fd_to_xfer.end());
  if(!nfds)
    return 0;
  
  memset(fds,0,nfds*(sizeof fds[0]));
  DBG(std::cout << "Watched file descriptors" << std::endl);

  unsigned j = 0;
  for(iterator i = begin(); i != end(); ++i){
    DBG(std::cout << (*i)->get_fd() << ":\t" << (*i)->desc() << ' ' << *i << std::endl);
    if(!(*i)->active())
      continue;
    if((*i)->has_timeout())
      has_timeout = true;

    fds[j].fd = (*i)->get_fd();
    fds[j].events = (*i)->get_stream_events();
    if(!fds[j].events)fds[j].events=(*i)->get_events();
    fds[j].events |= POLLHUP;
    if(_fd_to_xfer.find(fds[j].fd)!=_fd_to_xfer.end())
      errx(1,"internal error: two active IOContexts with same fd!");
    
    _fd_to_xfer[fds[j].fd] = *i;
    if(++j > nfds)
      errx(1,"internal error in XferTable::fill_fds, list changed under me!");
  }
  return j;
}

bool
XferTable::check_timeouts(std::time_t now, std::time_t& last_sweep,bool&has_timeout)
{
  bool rebuild_table = false;
  if(now > last_sweep + sweep_interval()){
    last_sweep = now;

    for(iterator i = begin(); i != end(); ++i){
      if((*i)->check_timeout(now,*this))
	rebuild_table = true;
      else if((*i)->has_timeout())
	has_timeout = true;
    }
  }
  return rebuild_table;
}

bool
XferTable::do_poll()
{
  size_t total_fds = size();
  bool rebuild_table = false;
  bool has_timeout;
  if(!total_fds)return true;
  
  struct pollfd *fds = new pollfd[total_fds];
  
  try{
    rebuild_table = false;
    has_timeout = false;

    size_t nfds = fill_fds(fds,total_fds,has_timeout);
    
    if(!nfds)
      goto out;

    std::time_t last_sweep = 0;
  
    for(;;){
      int events = ::poll(fds,nfds,has_timeout ? sweep_interval() : -1);

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

      if(reap())
	rebuild_table = true;
      if(check_timeouts(now,last_sweep,has_timeout))
	rebuild_table = true;
      
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

#include <cstdlib>

#include <err.h>
#include <assert.h>

#include "streamtable.hh"
#include "stream.hh"
#include "streamfd.hh"
#include "iocontext.hh"
#include "permitsignals.hh"

#undef debug
#define debug(x) do {}while(0)
#undef debug
#define debug(x) warnx x

void
StreamTable::remove(IOContext*ioc)
{
  debug(("removing iocontext %p: %s",ioc, ioc->desc().c_str()));
  for(_streams_type::iterator i = _streams.begin();i!=_streams.end();++i){
    if(!(*i)->consumer() || (*i)->consumer()==ioc)
      remove(*i);
  }
  for(_added_streams_type::iterator i = _added_streams.begin();i!=_added_streams.end();++i){
    if(!(*i)->consumer() || (*i)->consumer()==ioc)
      remove(*i);
  }
}


void
StreamTable::free()  {
  sow_and_reap();
  
  for(_streams_type::iterator i=_streams.begin();i!=_streams.end();i=_streams.erase(i))
    delete *i;
  //    _fd_to_stream.erase(_fd_to_stream.begin(),_fd_to_stream.end());
  //    _nonfd_streams.erase(_nonfd_streams.begin(),_nonfd_streams.end());
  _fd_to_stream.clear();
  _nonfd_streams.clear();
  _deleted_streams.clear();
  _added_streams.clear();
}

void
StreamTable::do_add(Stream*s)
{
  debug(("adding stream %p: %s",s,s->desc().c_str()));
  _streams.push_back(s);
  StreamFD*sfd = dynamic_cast<StreamFD*>(s);
  if(sfd){
    int fd = sfd->fd();
    _fd_to_stream[fd] = s;
  } else {
    _nonfd_streams.push_back(s);
  }
}

void
StreamTable::sow_and_reap()
{
  debug(("sowing and reaping"));
  // do not change the order!! as added streams might also be deleted
  for(_added_streams_type::iterator i=_added_streams.begin();
      i!=_added_streams.end();++i)
    do_add(*i);
  _added_streams.clear();
  
  for(_deleted_streams_type::iterator i=_deleted_streams.begin();
      i!=_deleted_streams.end();++i)
    do_remove(*i);
  _deleted_streams.clear();
  debug(("finished sowing and reaping"));
}


void
StreamTable::do_remove(Stream*s)
{
  debug(("really removing stream %p",s));
  _streams.remove(s);
  StreamFD*sfd = dynamic_cast<StreamFD*>(s);
  if(sfd){
    _fd_to_stream_type::iterator i = _fd_to_stream.find(sfd->fd());
    if(i != _fd_to_stream.end())
      _fd_to_stream.erase(i);
  } else {
    _nonfd_streams.remove(s);
  }
  debug(("delete starts"));
  try{
    delete s;
  } catch (const std::exception&e){
    warnx("exception raised deleting stream: %s",e.what());
  } catch (...){
    warnx("unknown exception raised deleting stream");
  }
  debug(("delete ends"));
}

bool
StreamTable::do_events(Stream*s)
{
  bool wantmore = false;
  debug(("polling %s:%s",s->desc().c_str(),s->consumer()?s->consumer()->desc().c_str():"<nul>"));
  try{
    if(s->consumer()){
      s->consumer()->read(*s);
    }
    if(s->ready_to_read() && s->consumer() && s->consumer()->want_read(*s))
      wantmore = true;
    if(s->consumer()){
      s->consumer()->write(*s);
    }
    if(s->ready_to_write() && s->consumer() && s->consumer()->want_write(*s))
      wantmore = true;
    
    if(!s->consumer()){
      remove(s);
      wantmore = false;
    }
  } catch (IOContext::Destroy&d){
    debug(("removing due to destroy %p",d.target()));
    remove(d.target());
    wantmore = false;
  }catch(const Stream::ClosedException&e){
    debug(("removing due to stream closed %p",s));
    remove(s);
    wantmore = false;
  } catch (const std::exception&e){
    warnx("closing %s:%s due to %s",s->desc().c_str(),s->consumer()?s->consumer()->desc().c_str():"<nul>",e.what());
    remove(s);
    wantmore = false;
  }
  return wantmore;
}


void
StreamTable::poll(int const*masked_signals)
{
  struct pollfd *fds = 0;
  unsigned fds_size = 0;
  if(0){
    // The simplest possible event loop
    
    while(!empty()){
      sow_and_reap();
      for(_streams_type::iterator i = _streams.begin();i!=_streams.end();++i)
	do_events(*i);
      {
	PermitSignals ps(masked_signals);
	sleep(1);
      }
    }
  }
  
  
  try{
    while(!empty()){
      bool nonfd_events_pending = false;
      sow_and_reap();
      for(_nonfd_streams_type::iterator i=_nonfd_streams.begin();
	  i!=_nonfd_streams.end();++i) {
	if(do_events(*i))
	  nonfd_events_pending = true;
	if((*i)->consumer()){
	  if((*i)->consumer()->check_timeout(**i)){
	    debug(("has timed out %s:%s",(*i)->desc().c_str(),(*i)->consumer()?(*i)->consumer()->desc().c_str():"<nul>"));
	  }
	}
      }
      sow_and_reap();
      {
	unsigned nfds = _fd_to_stream.size();
	if(nfds != fds_size){
	  if(nfds) {
	    fds = (struct pollfd*)realloc(fds,nfds*sizeof(fds[0]));
	    if(!fds)
	      errx(EXIT_FAILURE,"out of memory allocating poll buffer");
	  } else {
	    if(fds) {
	      ::free(fds);
	      fds = 0;
	    }
	  }
	  fds_size = nfds;
	}
      }
      int fd=0;
      for(_fd_to_stream_type::iterator i = _fd_to_stream.begin();
	  i!=_fd_to_stream.end();++i,++fd){
	fds[fd].fd = i->first;
      
	Stream*s = i->second;
	fds[fd].events = POLLHUP;
	try{
	  if(s->consumer()){
	    if(s->consumer()->check_timeout(*s)){
	      debug(("has timed out %s:%s",s->desc().c_str(),s->consumer()?s->consumer()->desc().c_str():"<nul>"));
	    }
	  }
	  if(s->consumer()) {
	    if(s->consumer()->want_read(*s)) {
	      debug(("wants read %s:%s",s->desc().c_str(),s->consumer()?s->consumer()->desc().c_str():"<nul>"));
	      fds[fd].events |= POLLIN;
	    }
	  }
	  if(s->consumer()) {
	    if(s->consumer()->want_write(*s)){
	      debug(("wants write %s:%s",s->desc().c_str(),s->consumer()?s->consumer()->desc().c_str():"<nul>"));
	      fds[fd].events |= POLLOUT;
	    }
	  }

	} catch (IOContext::Destroy&d){
	  debug(("removing fd due to destroy %p",d.target()));
	  remove(d.target());
	}
      }
      if(tables_modified())
	continue;
      assert((unsigned)fd==fds_size);

      if(_async_notify)
	break;
      int events;
      {
	PermitSignals ps(masked_signals);
	events = ::poll(fds,fds_size,nonfd_events_pending?0:-1);
      }

      for(unsigned i=0;events && i < fds_size;++i)
	if(fds[i].revents){
	  Stream*s=_fd_to_stream[fds[i].fd];
	  do_events(s);
	  if(fds[i].revents&POLLHUP){
	    debug(("scheduling %p for removal",s));
	    remove(s);
	  }
	  --events;
	}
    }
  } catch (...){
    if(fds) {
      ::free(fds);
      fds = 0;
    }
    throw;
  }
  if(fds) {
    ::free(fds);
    fds = 0;
  }
}


#include <exception>
#include <list>

#include <cstdlib>
#include <ctime>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#endif

#include <streamtable.hh>
#include <dircontainer.hh>
#include <confexception.hh>
#include <unixexception.hh>
#include "serverregistration.hh"

ServerRegistration* ServerRegistration::registered_list;

static
void
load_servers(StreamTable&xt,const std::string&path,Server::Factory factory)
{
  
  DirContainer configs(path);

  for(DirContainer::iterator i = configs.begin(); i != configs.end(); ++i){
    Server *fl = factory();
    std::string cfg = configs.name() + "/" + *i;
    fl->stream_container(&xt);
    try {
      xt.add(fl->read_config(cfg));
    }
    catch(std::exception&e){
      warnx("%s",e.what());
      warnx("server config \"%s\" bad",cfg.c_str());
      delete fl;
    }
  }
}

static
void
start_servers(const std::string&configdir)
{
  StreamTable xt;

  for(ServerRegistration* i=ServerRegistration::registered_list;
      i;i=i->next()){
    load_servers(xt,configdir+ "/" + (*i).name(),(*i).factory());
  }
  
  if(xt.empty())
    errx(1,"no valid servers specified in \"%s\"",configdir.c_str());
  
  xt.poll();
  errx(1,"all servers died");
}

static
void
set_signals()
{
  if(signal(SIGPIPE,SIG_IGN) == SIG_ERR)
    throw UnixException("signal(SIG_PIPE,SIG_IGN)");
}

static
void
set_umask()
{
  // allow us to create world writable files or else teepeedee will
  // still create files in group writable directories, but will refuse
  // to write to them as they have too restrictive write permissions.

  umask(0002);
}

static
void
setup_openssl()
{
#ifdef HAVE_OPENSSL  
  SSL_load_error_strings();
  SSL_library_init();
#endif
}


int main(int argc,char*argv[])
{
  try{
    std::srand(std::time(0));
    set_signals();
    set_umask();
    setup_openssl();
    start_servers((argc > 1 && argv[1]) ? argv[1] : TPD_CONFIGDIR);
  } catch(std::bad_exception&be){
    errx(1,"bad exception: %s",be.what());
  } catch (ConfException&ce){
    errx(1,"misconfiguration: %s",ce.what());
  }
  catch(std::exception&e){
    warnx("%s",e.what());
    errx(1,"exiting due to uncaught exception");
  }
  catch(...){
    errx(1,"uncaught unknown exception");
  }

  return 1;
}

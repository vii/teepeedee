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

#include <signal.hh>
#include <streamtable.hh>
#include <dircontainer.hh>
#include <confexception.hh>
#include <unixexception.hh>
#include "confdb.hh"
#include "conf.hh"
#include "serverregistration.hh"

ServerRegistration* ServerRegistration::registered_list;

static
void
load_servers(StreamTable&xt,Conf&conf,Server::Factory factory)
{
  typedef std::list<ConfDB::Key> config_keys_t;
  config_keys_t config_keys;
  conf.get_subkeys(config_keys);
  for(config_keys_t::iterator i = config_keys.begin();i!=config_keys.end();++i) {
    Conf sc(conf,*i);
    Server *fl = factory(sc);
    fl->stream_container(&xt);
    try {
      xt.add(fl->read_config());
    }
    catch(std::exception&e){
      warnx("%s",e.what());
      warnx("server config \"%s\" bad",sc.get_name().c_str());
      delete fl;
    }
  }
}

static
StreamTable*running_table;

static
void
stop_table(int signal)
{
  // strsignal is a GNU extension
  warnx("received signal %d, shutting down",signal);
  
  if(running_table)
    running_table->async_notify_stop();
}

static
ConfDB*running_db;
static
void
sync_conf_db(int signum)
{
  warnx("received signal %d, sync'ing configuration",signum);
  try{
    if(running_db)
      running_db->sync();
  } catch(const std::exception&e){
    warnx("error sync'ing: %s",e.what());
  }catch (...){
    warnx("caught unknown exception sync'ing configuration");
    abort();
  }
}

static
void
start_servers(const std::string&configdir)
{
  ConfDB db(configdir);
  
  {
    running_db = &db;
    StreamTable xt;

    for(ServerRegistration* i=ServerRegistration::registered_list;
	i;i=i->next()){
      Conf sc(db,i->name());
      load_servers(xt,sc,(*i).factory());
    }
  
    if(xt.empty())
      errx(1,"no valid servers specified in \"%s\"",configdir.c_str());

    running_table = &xt;
    {
      const int signals[] = { SIGINT, SIGTERM, SIGUSR1, NSIG 
      };
      sigset_t ss;
      sigemptyset(&ss);
      for(int const*i=signals;*i!=NSIG;++i)
	sigaddset(&ss,*i);
      sigprocmask(SIG_BLOCK,&ss,0);

      Signal sigint(SIGINT,stop_table);
      Signal sigterm(SIGTERM,stop_table);
      Signal sigusr1(SIGUSR1,sync_conf_db);
      
      xt.poll(signals);
      
      running_db = 0;
    }
    running_table = 0;
  }
  
  db.sync();
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

  return 0;
}

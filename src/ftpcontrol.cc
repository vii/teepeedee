#include <sstream>

#include <cctype>

#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>

#include <xfertable.hh>
#include <unixexception.hh>
#include <confexception.hh>
#include <iocontextxfer.hh>
#include <filelisterglob.hh>

#include "ftpcontrol.hh"
#include "ftpdatalistener.hh"
#include "ftplist.hh"
#include "ftplistlong.hh"
#include "ftplistmlsd.hh"
#include "ftpmlstprinter.hh"

#define cmd(name,desc) { #name, & FTPControl::cmd_ ## name , desc }
const FTPControl::ftp_cmd FTPControl::ftp_cmd_table[] = {
  cmd(user," <username>: initate authentication as username, follow with pass"),
  cmd(pass," <password>: authenticate as previously given username"),
  cmd(help," [<command>]: get help (possibly about a command) from the server"),
  cmd(rein,":  cancel authentication"),
  cmd(quit,": terminate the session"),
  cmd(noop,": do nothing"),
  cmd(syst,": get information about the system type"),
  cmd(type," <type>: change RFC959 transfer type"),
  cmd(mode," <type>: change RFC959 transfer mode"),
  cmd(stru," <type>: change RFC959 transfer structure"),
  cmd(pasv,": enter passive mode (server listens for data connection)"),
  cmd(pwd,": print current directory"),
  cmd(abor,": abort transfer in progress"),
  cmd(cdup,": move up to the parent directory"),
  cmd(nlst," [<directory>]: list filenames in current or specified directory"),
  cmd(list," [<directory or filename>]: list information about files or directories"),
  cmd(rnfr," <filename>: prepare to rename this file, follow with rnto"),
  cmd(rnto," <filename>: rename file specified with rnfr to this one"),
  cmd(cwd," <directory>: change to directory"),
  cmd(dele," <filename>: delete file"),
  cmd(port,": enter active mode - server connects to client for data connection"),
  cmd(retr," <filename>: retrieve a file from the server"),
  cmd(stor," <filename>: store a file on the server"),
  cmd(rest," <position>: start next transfer a certain offset into the file"),
  cmd(mkd, "<directory>: create directory"),
  cmd(rmd, "<directory>: remove empty directory"),
  cmd(stat, ": inform whether a transfer is taking place"),
  cmd(mlst," <filename>: machine readable directory listing of filename"),
  cmd(mlsd," [<directory>]: machine readable directory listing of directory"),
  cmd(feat,": list optional features supported (as in draft-ietf-ftpext-mlst-16.txt)"),
  cmd(mdtm," <filename>: list time file was modified"),
  cmd(size," <filename>: list size of file"),
  {0,0,0}
};
#undef cmd


FTPControl::FTPControl(int fd,const ConfTree&conf)
  :
  _conf(conf),
  _authenticated(false),
  _data_listener(0),
  _data_xfer(0),
  _restart_pos(0),
  _quitting(false)
{
  set_fd(fd);
  std::string greet;
  
  {
    try{
      getsockname(_port_addr);
    } catch (UnixException&ue){
      warnx("getsockname on ftp control fd: %s",ue.what());
    }
    _port_addr.sin_port = htons(21);
  }
  
  _conf.get_line("greeting",greet);
  _conf.get_timeout("timeout_prelogin",*this);
  make_response("220",greet);
}

FTPControl::events_t
FTPControl::get_events()
{
  if(_quitting && !closing()){
    return response_ready() ? POLLOUT : 0;
  }
  return IOContextResponder::get_events();
}

void
FTPControl::xfer_done(IOContextControlled*xfer,bool success)
{
  if(xfer != _data_xfer)
    warnx("internal error in FTPControl::xfer_done");
  else{
    _data_xfer = 0;
    if(_quitting)
      close_after_output();
      
    if(success)
      make_response("226","Transfer complete");
    else
      make_response("426","Transfer closed due to error: timeout?");
  }
  _conf.get_timeout("timeout_postlogin",*this);
}

void
FTPControl::start_xfer(XferTable&xt,IOContextControlled*xfer)
{
  if(passive()){
    _data_listener->set_data(xfer);
  } else {
    xfer->become_ipv4_socket();
    xfer->set_nonblock();
    try{
      xfer->set_bind_reuseaddr();
      struct sockaddr_in sai;
      getsockname(sai);
      xfer->bind_ipv4(sai.sin_addr.s_addr,htons(ntohs(sai.sin_port)-1));
    } catch (UnixException&ue){
      warnx("unable to set local transfer port: %s",ue.what());
    }
    
    int ret = connect(xfer->get_fd(),(sockaddr*)&_port_addr,sizeof _port_addr);
    if(ret == -1){
      if(errno != EINPROGRESS)
	throw UnixException("connect to user port");
    }
    xt.add(xfer);
  }

  _conf.get_timeout("timeout_xfer",*xfer);
  set_timeout_interval(0);
  
  _data_xfer = xfer;
  make_response("150","The data is in the post");
}

static
int
get_port_desc_byte(const char*&s)
{
  char*next;
  int ret = strtoul(s,&next,10);
  if(s == next)
    s = 0;
  else
    s = next;
  if(ret != (ret & 0xff))
    s = 0;
  return ret;
}

void
FTPControl::cmd_dele(XferTable&xt,const std::string&argument)
{
  assert_auth();
  if(!_user.unlink(Path(_path,argument))){
    make_response("450","It's invincible!");
  }else
    make_response("250","Destruction complete");
}

void
FTPControl::cmd_rest(XferTable&xt,const std::string&argument)
{
  assert_auth();
  const char*s = argument.c_str();
  char*r;
  
  _restart_pos = strtoull(s,&r,10);
  if(r == s){
    make_response("501","Improve your figure");
  }else
    make_response("350","Your wish is my command");
}

bool
FTPControl::restart(int fd)
{
  off_t pos = take_restart_pos();
  
  if(pos && lseek(fd,pos,SEEK_SET)==-1){
    ::close(fd);
    make_response("553","Cannot find the rest position");
    return false;
  }
  return true;
}


void
FTPControl::cmd_retr(XferTable&xt,const std::string&argument)
{
  assert_auth();
  assert_no_xfer();
  int fd = _user.open(Path(_path,argument),O_RDONLY);
  if(fd == -1){
    make_response("550","File not found");
    return;
  }
    
  if(!restart(fd))
    return;
  
  IOContextXfer *fdr = new IOContextXfer(this);
  fdr->set_read_fd(fd);
  start_xfer(xt,fdr);
  return;
}
void
FTPControl::cmd_stor(XferTable&xt,const std::string&argument){
  assert_auth();
  assert_no_xfer();
  int fd = _user.open(Path(_path,argument),O_WRONLY|O_CREAT);
  if(fd == -1){
    make_response("550","File not found");
    return;
  }
  if(!restart(fd))
    return;

  IOContextXfer *fdr = new IOContextXfer(this);
  fdr->set_write_fd(fd);
  
  start_xfer(xt,fdr);
  return;
}

void
FTPControl::cmd_port(XferTable&xt,const std::string&argument)
{
  assert_auth();
  assert_no_xfer();
  uint32_t haddr=0;
  uint16_t hport=0;
  const char*s=argument.c_str();
  for(int i=3;i>=0;--i){
    haddr |= get_port_desc_byte(s) << (i*8);
    if(!s || *s++ != ','){
      goto syn_error;
    }
  }
  hport |= get_port_desc_byte(s) << 8;
  if(!s || *s++ != ','){
    goto syn_error;
  }
  hport |= get_port_desc_byte(s);
  if(!s)goto syn_error;
  
  bool cancel;
  _conf.get("no_port_mode",cancel);
  
  if(cancel || !passive_off(xt)){
    make_response("502","Hey! I'm the passive partner here");
    return;
  }
  _port_addr.sin_family = AF_INET;
  _port_addr.sin_addr.s_addr = htonl(haddr);
  _port_addr.sin_port = htons(hport);
  make_response("200","Port change ok, captain");
  return;
  
 syn_error:
  make_response("501","Bad port description");
}


void
FTPControl::do_cmd_list(XferTable&xt,const std::string&argument,bool detailed)
{ // Messy hack central. None of this is spec'd in the RFC 959 but clients expect it
  assert_auth();
  assert_no_xfer();
  std::string arg = argument;

  FileLister*fl;

  // ignore -options from stupid clients [like gftp] who try to
  // send us flags to ls(1)
  while(!arg.empty() && arg[0] == '-') {
    std::string::size_type s = arg.find(' ');
    if(s != std::string::npos)
      arg = arg.substr(s+1);
    else
      arg = std::string();
  }
  
  if(arg.find_first_of("*?")!=std::string::npos)
    fl = new FileListerGlob(arg,_user.list(Path(_path)));
  else
    fl = _user.list(Path(_path,arg));

  IOContextControlled*fdl;
  try{
    fdl = detailed ? new FTPListLong(this,_user,fl) : new FTPList(this,fl);
  } catch (...){
    delete fl;
    throw;
  }
  start_xfer(xt,fdl);
  
  return;
}

void
FTPControl::cmd_abor(XferTable&xt,const std::string&argument)
{
  assert_auth();
  if(xfer_pending()){
    make_response("426","Terminating due to abort");
    IOContextControlled*dx = _data_xfer;
    dx->successful();
    dx->discard_hangup(xt);
  } else
    make_response("226","Nothing to abort");
  return;
}

void
FTPControl::cmd_feat(XferTable&xt,const std::string&argument)
{// defined in draft-ietf-ftpext-mlst-16.txt
  if(!argument.empty()){
    make_response("504","FEAT not implemented with argument");
    return;
  }
  make_response("211-","The following features are supported\r\n"
		"  MDTM\r\n"
		"  SIZE\r\n"
		"  MLST Type*;Size*;Modify*;Perm*\r\n"
		"  REST STREAM\r\n"
		"  TVFS"
		);
  make_response("211","That is all");
}

void
FTPControl::cmd_mdtm(XferTable&xt,const std::string&argument)
{
  assert_auth();

  User::Stat buf;
  Path p(_path,argument);
  if(!_user.stat(p,buf)){
    make_response("550","Path does not exist");
    return;
  }

  std::string mtime = FTPMlstPrinter::mtime(buf.mtime());
  make_response("213",mtime);
}
  
void
FTPControl::cmd_size(XferTable&xt,const std::string&argument)
{
  assert_auth();

  User::Stat buf;
  Path p(_path,argument);
  if(!_user.stat(p,buf)){
    make_response("550","File does not exist");
    return;
  }
  if(buf.is_dir()) {
    make_response("550","Pathname is directory");
    return;
  }

  std::ostringstream s;
  s << buf.size();
  make_response("213",s.str());
}

void
FTPControl::cmd_mlst(XferTable&xt,const std::string&argument)
{
  assert_auth();

  User::Stat buf;
  Path p(_path,argument);
  if(!_user.stat(p,buf)){
    make_response("550","File does not exist");
    return;
  }

  FTPMlstPrinter mlst(_user);
  make_response("250-","MLST listing\r\n   "
		+ mlst.line(p)
		);
  make_response("250","MLST finished");
}

void
FTPControl::FTPControl::cmd_mlsd(XferTable&xt,const std::string&argument)
{
  assert_auth();
  assert_no_xfer();

  FileLister*fl = _user.list(Path(_path,argument));

  IOContextControlled*fdl;
  FTPMlstPrinter mlst(_user);
  try{
    fdl = new FTPListMlsd(this,fl,mlst);
  } catch (...){
    delete fl;
    throw;
  }
  start_xfer(xt,fdl);
}

void
FTPControl::cmd_help(XferTable&xt,const std::string&argument) {
  std::string help = "Commands";
  for(const ftp_cmd*i=ftp_cmd_table;i->name;++i)
    help += std::string("\r\n   ") + i->name + std::string(i->desc);

  make_response("214-",help);
  make_response("214","No more help available");
}

void
FTPControl::cmd_cwd(XferTable&xt,const std::string&argument)
{
  assert_auth();
  Path p(_path,argument);
    
  if(!_user.may_chdir(p)){
    make_response("550","You cannot or may not chdir there");
    return;
  }
  _path = p;
  make_response("250","Directory changed");
  return;
}

void
FTPControl::make_response(const std::string&code,const std::string&str)
{
  std::string total = code + " " + str + "\r\n";

  add_response(total);
}

FTPControl::~FTPControl()
{
}

void
FTPControl::finished_reading(XferTable&xt,char*buf,size_t len)
{
  using std::isspace; // damn gcc-2.95.2
  using std::iscntrl;
  
  std::string name(buf,len); // yes guaranteed nul terminated
  std::string argument;
  
  for(size_t i = 0; i < len;++i){
    if(isspace(buf[i])){
      buf[i] = 0;
      name = buf;
      if(++i!=len){
	argument = buf + i;
      }
      break;
    }
  }

  for(std::string::iterator i=name.begin();i!=name.end();){
    if(!isalnum(*i)) // be lazy and ignore control sequences
      i=name.erase(i);
    else{
      *i = std::tolower(*i);      
      ++i;
    }
  }

  try{
    command(name,argument,xt);
    return;
  } catch (UnixException&e){
    warnx("internal error: %s",e.what());
  }
      
  make_error("Server too confused to process command");
}

bool
FTPControl::login(const std::string&password)
{
  std::string uname = _username;
  if(uname.empty())
    return false;
  lose_auth();
  try{
    ConfTree users;
    _conf.get("users",users);
    
    if(_user.authenticate(uname,password,users)){
      _authenticated = true;

      _conf.get_timeout("timeout_postlogin",*this);
      
      return true;
    }
    
  } catch (std::exception&e){
    warnx("error for user trying to login: %s",e.what());
  }
  
  return false;
}



void
FTPControl::command(const std::string&cmd,const std::string& argument,XferTable&xt)
{
  try{
    for(const ftp_cmd*i=ftp_cmd_table;i->name;++i)
      if(cmd == i->name) {
	(this->*i->function)(xt,argument);
	return;
      }
  } catch (User::AccessException&){
    make_response("550","I'm sorry, I can't let you do that");
    return;
  } catch ( UnauthenticatedException& ) {
    make_response("530","Who do you think you are?");
    return;
  } catch ( XferPendingException& ){
    make_response("451","Cool it, greedy! You already have one transfer on the go");
    return;
  } catch (ConfException&ce){
    make_error("Server misconfigured: " + std::string(ce.what()));
    return;
  }
  
  make_response("502","Come again?");
  return;
}

bool
FTPControl::passive_off(XferTable&xt)
{
  if(passive()){
    _data_listener->hangup(xt);
    _data_listener = 0;
    return true;
  }
  return true;
}

static
std::string
comma_dump_addr(const struct sockaddr_in& addr)
{
  std::ostringstream s;
  uint32_t ad = ntohl(addr.sin_addr.s_addr);
  uint16_t port = ntohs(addr.sin_port);
  for(int i = 3;i>=0;--i)
    s << unsigned(ad >> (i*8) & 0xff) << ',';
  s << unsigned((port >> 8)&0xff) << ',' << unsigned(port & 0xff);
  return s.str();
}

bool
FTPControl::passive_on(XferTable&xt)
{
  // always reinitialize passive mode as the accept queue created by the
  // FTPDataListener might have been mangled by the client
  
  // XXX is it not sufficient to simply eat all the accepts? - I'm
  // worried about possibility of delayed packets coming
  
  if(passive())
    passive_off(xt);
  
  bool cancel;
  _conf.get("no_passive_mode",cancel);
  if(cancel)
    return false;
  int px = 65535,pn = IPPORT_RESERVED;
  if(_conf.exists("passive_port_min")){
    _conf.get("passive_port_min",pn);
  }
  if(_conf.exists("passive_port_max")){
    _conf.get("passive_port_max",px);
  }


  _data_listener = new FTPDataListener;
  try{
    struct sockaddr_in sai;
    getsockname(sai);
    
    _data_listener->bind_ipv4(sai.sin_addr.s_addr,pn,px);
    _data_listener->listen();
  } catch (UnixException&ue){
    if(_data_listener){
      delete _data_listener;
      _data_listener = 0;
    }
    warnx("unable to bind passive listener: %s",ue.what());
    return false;
  }
  xt.add(_data_listener);

  struct sockaddr_in addr;
  _data_listener->getsockname(addr);

  make_response("227",std::string("Entering Passive Mode. (")+
		comma_dump_addr(addr)
    +")");
  return true;
}


void
FTPControl::hangup(class XferTable&xt)
{
  if(passive())
    passive_off(xt);
  if(xfer_pending())
    _data_xfer->detach();

  super::hangup(xt);
}


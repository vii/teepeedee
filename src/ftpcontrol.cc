#include <sstream>

#include <cctype>
#include <cstdlib>

#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>

#include <sslstreamfactory.hh>
#include <unixexception.hh>
#include <confexception.hh>
#include <iocontextxfer.hh>
#include <filelisterglob.hh>

#include "xferlimituser.hh"
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
  cmd(eprt," <d><protocol number><d><address><d><port><d>: extended active mode"),
  cmd(epsv," <protocol number>|ALL: extended passive mode"),
  cmd(auth," TLS: turn the session into an SSL session"),
  cmd(pbsz," 0: comply with RFC 2228 before issuing PROT"),
  {0,0,0}
};
#undef cmd


FTPControl::FTPControl(const Conf&conf,StreamContainer&sc,SSLStreamFactory*ssf)
  :
  Control(conf,sc),
  _data_listener(0),
  _data_xfer(0),
  _ssl_factory(ssf),
  _restart_pos(0),
  _quitting(false),
  _turning_into_ssl(false),
  _unencrypted_stream(0)
{
  std::string greet = "Service ready for new user.";
  config().get_if_exists("greeting",greet);
  config().get_timeout("timeout_prelogin",*this);
  make_response_multiline("220",greet);
}

void
FTPControl::remote_stream(const Stream&stream)
{
  try{
    stream.getpeername(_port_addr);
    _port_addr.sin_port = htons(20);
    stream.getsockname(_local_addr);
  } catch (const std::exception&e){
    warnx("unable to get port or pasv details: %s",e.what());
    make_error("Server not on network");
  }
  super::remote_stream(stream);
}


void
FTPControl::xfer_done(IOContextControlled*xfer,bool success)
{
  if(xfer == _data_xfer) {
    _data_xfer = 0;
    if(_data_listener&&_data_listener->has_data()){
      _data_listener->release_data();
      _data_listener = 0;
    }
      
    if(_quitting)
      close_after_output();
      
    if(success)
      make_response("226","Transfer complete");
    else
      make_response("426","Transfer closed due to error: timeout?");
    config().get_timeout("timeout_postlogin",*this);
  } else if (xfer == _data_listener) {
    _data_listener = 0;
  }
}

void
FTPControl::start_xfer(IOContextControlled*xfer,Stream*fd)
{
  if(passive()){
    _data_listener->set_data(xfer);
  } else {
    sockaddr_in sai(_local_addr);
    sai.sin_port = htons(ntohs(sai.sin_port)-1);
    Stream*s = Stream::connect((const sockaddr*)&_port_addr,sizeof _port_addr,(const sockaddr*)&sai,sizeof sai);
    s->consumer(xfer);
    stream_container().add(s);
  }

  _data_xfer = xfer;
  if(fd)
    stream_container().add(fd);
  config().get_timeout("timeout_xfer",*xfer);
  set_timeout_interval(0);
  
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
FTPControl::cmd_dele(const std::string&argument)
{
  assert_auth();
  if(!_user.unlink(Path(_path,argument))){
    make_response("450","It's invincible!");
  }else
    make_response("250","Destruction complete");
}

void
FTPControl::cmd_rest(const std::string&argument)
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
FTPControl::restart(Stream*fd)
{
  off_t pos = take_restart_pos();
  
  if(pos){
    try{
      fd->seek_from_start(pos);
    } catch (...){
      make_response("553","Cannot find the rest position");
      return false;
    }
  }
  return true;
}

void
FTPControl::start_file_xfer(const Path&path,
			    XferLimit::Direction::type dir)
{
  Stream*fd;
  try{
    fd = _user.open(path,dir == XferLimit::Direction::upload ? O_WRONLY|O_CREAT
		    : O_RDONLY);
  } catch (std::exception&e){
    make_response("550","File not there");
    return;
  }

  IOContextXfer*ioc;
  if(!restart(fd)){
    delete fd;
    return;
  }
  try{
    ioc = new IOContextXfer(this);
  } catch (...){
    delete fd;
    throw;
  }
  fd->consumer(ioc);
  if(dir == XferLimit::Direction::download)
    ioc->stream_in(fd);
  else
    ioc->stream_out(fd);
    
  try {
    ioc->limit(new XferLimitUser(_user,dir,remotename(),"ftp",
				 path.str()));
  } catch (...){
    fd->release_consumer();
    delete ioc;
    delete fd;
    throw;
  }
  start_xfer(ioc,fd);
}


void
FTPControl::cmd_retr(const std::string&argument)
{
  assert_auth();
  assert_no_xfer();
  start_file_xfer(Path(_path,argument),XferLimit::Direction::download);
}
void
FTPControl::cmd_stor(const std::string&argument){
  assert_auth();
  assert_no_xfer();
  start_file_xfer(Path(_path,argument),XferLimit::Direction::upload);
}

bool FTPControl::active_on()
{
  bool cancel;
  config().get("no_port_mode",cancel);
  
  if(cancel || !passive_off()){
    make_response("502","Hey! I'm the passive partner here");
    return false;
  }
  return true;
}


void
FTPControl::cmd_port(const std::string&argument)
{
  assert_auth();

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

  if(!active_on()){
    return;
  }
  memset(&_port_addr,0,sizeof _port_addr);
  _port_addr.sin_family = AF_INET;
  _port_addr.sin_addr.s_addr = htonl(haddr);
  _port_addr.sin_port = htons(hport);
  make_response("200","Port change ok, captain");
  return;
  
 syn_error:
  make_response("501","Bad port description");
}


void
FTPControl::do_cmd_list(const std::string&argument,bool detailed)
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
  start_xfer(fdl);
  
  return;
}

void
FTPControl::cmd_abor(const std::string&argument)
{
  assert_auth();
  if(xfer_pending()){
    make_response("426","Terminating due to abort");
    IOContextControlled*dx = _data_xfer;
    dx->completed();
    throw Destroy(dx);
  } else
    make_response("226","Nothing to abort");
  return;
}

void
FTPControl::cmd_feat(const std::string&argument)
{// defined in draft-ietf-ftpext-mlst-16.txt
  if(!argument.empty()){
    make_response("504","FEAT not implemented with argument");
    return;
  }
  make_response("211-","The following features are supported\r\n"
		"  AUTH TLS\r\n"
		"  PBSZ\r\n"
		"  MDTM\r\n"
		"  SIZE\r\n"
		"  MLST Type*;Size*;Modify*;Perm*\r\n"
		"  REST STREAM\r\n"
		"  TVFS"
		);
  make_response("211","That is all");
}

void
FTPControl::cmd_mdtm(const std::string&argument)
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
FTPControl::cmd_size(const std::string&argument)
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
FTPControl::cmd_mlst(const std::string&argument)
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
FTPControl::FTPControl::cmd_mlsd(const std::string&argument)
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
  start_xfer(fdl);
}

void
FTPControl::cmd_help(const std::string&argument) {
  std::string help;
  if(argument.empty())
     help = "Commands";
  else
    help = "Help for command " + argument;
  
  for(const ftp_cmd*i=ftp_cmd_table;i->name;++i)
    if(argument.empty() || argument == i->name)
      help += std::string("\r\n   ") + i->name + std::string(i->desc);

  make_response("214-",help);
  make_response("214","No more help available");
}

void
FTPControl::cmd_cwd(const std::string&argument)
{
  assert_auth();
  Path p(_path,argument);
    
  if(!_user.may_chdir(p)){
    make_response("550","You cannot or may not chdir there");
    return;
  }
  _path = p;
  make_response_multiline("250","Directory changed magnificently\n"+_user.message_directory(_path));
  return;
}

void
FTPControl::make_response(const std::string&code,const std::string&str,char connect)
{
  std::string total = code + connect + str + "\r\n";

  add_response(total);
}
void
FTPControl::make_response_multiline(const std::string&code,const std::string&multiline)
{
  std::string::size_type size = multiline.size();
  std::string::size_type start,i;
  for(start=0,i=0;i<size;++i){
    if(!multiline[i]) {
      goto safe_i;
    }
    if(multiline[i]=='\n'){
      if(i+1==size || !multiline[i+1])goto safe_i;
      if(start){
	make_response(std::string(),multiline.substr(start,i-start));
      } else {
	make_response(code,multiline.substr(start,i-start),'-');
      }
      start = i+1;
    }
  }
  if(i)--i;
 safe_i:
  make_response(code,multiline.substr(start,i-start));
}     

FTPControl::~FTPControl()
{
  if(xfer_pending()){
    _data_xfer->detach();
    _data_xfer = 0;
  }
  passive_off();
}

void
FTPControl::finished_reading(char*buf,size_t len)
{
  using std::isspace; // damn gcc-2.95.2 on debian
  using std::iscntrl; // damn gcc-2.95.2 on debian
  using std::tolower; // for NetBSD 1.6
  
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
      *i = tolower(*i);      
      ++i;
    }
  }

  try{
    command(name,argument);
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

  bool success;
  if(uname == "ftp" || uname == "anonymous")
    success = user_login_default(_user,password);
  else
    success = user_login(_user,uname,password);

  if(success){
    config().get_timeout("timeout_postlogin",*this);
    return true;
  }
  
  return false;
}



void
FTPControl::command(const std::string&cmd,const std::string& argument)
{
  try{
    for(const ftp_cmd*i=ftp_cmd_table;i->name;++i)
      if(cmd == i->name) {
	(this->*i->function)(argument);
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
FTPControl::passive_off()
{
  if(passive()){
    _data_listener->free();
    _data_listener->detach();
    _data_listener = 0;
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
FTPControl::passive_on(bool epsv)
{
  // always reinitialize passive mode as the accept queue created by the
  // FTPDataListener might have been mangled by the client
  if(passive())
    passive_off();
  
  bool cancel;
  config().get("no_passive_mode",cancel);
  if(cancel)
    return false;
  int px = 65535,pn = IPPORT_RESERVED;
  config().get_if_exists("passive_port_min",pn);
  config().get_if_exists("passive_port_max",px);

  _data_listener = new FTPDataListener(this);
  _data_listener->stream_container(&stream_container());
  Stream*s;
  try{
    s=Stream::listen_ipv4_range(_local_addr.sin_addr.s_addr,pn,px);
    s->consumer(_data_listener);
  } catch (...){
    delete _data_listener;
    _data_listener = 0;
    warnx("unable to bind passive listener");
    return false;
  }
  stream_container().add(s);
  config().get_timeout("timeout_xfer",*_data_listener);

  sockaddr_in addr;
  s->getsockname(addr);
  if(epsv){
    std::ostringstream oss;
    oss << ntohs(addr.sin_port) << "|)";
    make_response("229","Entering Extended Passive Mode (|||"+oss.str());
  }else
    make_response("227",std::string("Entering Passive Mode. (")+
		comma_dump_addr(addr)
    +")");
  return true;
}


bool
FTPControl::stream_hungup(Stream&stream)
{
  if(passive())
    passive_off();
  if(xfer_pending())
    _data_xfer->detach();

  super::stream_hungup(stream);
  return true;
}

void
FTPControl::cmd_pass(const std::string&argument)
{
  if(_username.empty()){
    make_response("503","Who's there?");
    return;
  }
  try {
    if(!login(argument)){
      make_response("530","You are not everything you claim to be");
    } else {
      make_response_multiline("230",_user.message_login());
    }
  } catch (const LimitException&){
    make_response("530","Too many users in this class are logged in, try again later");
  }
}

void
FTPControl::cmd_epsv(const std::string&argument)
{
  assert_auth();

  if(argument.empty()
     || !strcasecmp(argument.c_str(),"1")){
    passive_on(true);
    return;
  }
  if(!strcasecmp(argument.c_str(),"all")) {
    make_response("501","RFC 2428 makes proper support for EPSV ALL hasslesome so it isn't suppored");
    return;
  }
  make_response_proto_not_supported();
}

void
FTPControl::cmd_eprt(const std::string&argument)
{
  assert_auth();
  if(argument.empty()) {
    make_response("501","Empty extended port argument - time for some sherry?");
    return;
  }

  if(!active_on())
    return;
  memset(&_port_addr,0,sizeof _port_addr);
  
  const char*str=argument.c_str();
  char delim = *str++;
  char prot = *str++;
  if(prot == 0||prot ==delim){
    make_response("501","No protocol specified for port (pass it left)");
    return;
  }
  switch(prot){
    case '1':
      _port_addr.sin_family = AF_INET;
      break;
  default:
    make_response_proto_not_supported();
    return;
  }
  if(*str++ != delim){
    make_response_proto_not_supported();
  }
  std::string address;
  while(*str && *str!=delim)
    address += *str++;

  if(inet_pton(_port_addr.sin_family,address.c_str(),&_port_addr.sin_addr.s_addr)<=0){
    make_response("501","Invalid address, wrong side of town");
    return;
  }
  
  if(!*str++){
    make_response("501","No port? No cigars? Call this a dinner party?");
    return;
  }
  
  char*tail;
  unsigned long port = strtoul(str,&tail,10);
  if(tail == str || *tail != delim){
    make_response("501","Bad port received (leading to gout)");
    return;
  }
  _port_addr.sin_port = htons(port);
  make_response("200","Always happy to see a new port");
}

void
FTPControl::cmd_auth(const std::string&argument)
{
  if(!strcasecmp(argument.c_str(),"tls")
     ||!strcasecmp(argument.c_str(),"tls-c")) {
    if(!_ssl_factory){
      make_response("534","Not configured for TLS (no keys?)");
      return;
    }
    make_response("234","Now happily talking TLS");
    _turning_into_ssl = true;
    return;
  }
  make_response("504","Only know about AUTH TLS");
}
void
FTPControl::read_in(Stream&stream,size_t max)
{
  if(_turning_into_ssl&&!response_ready()){
    _turning_into_ssl = false;
    if(!_unencrypted_stream){
      _unencrypted_stream = &stream;
    }
    Stream*ns;
    IOContext*saved = stream.consumer();
    stream.release_consumer();
    try{
      ns=_ssl_factory->new_stream(&stream,stream_container());
    } catch (...){
      stream.consumer(saved);
      throw;
    }
    ns->consumer(saved);
    stream_container().add(ns);
  } else
    super::read_in(stream,max);
}

void
FTPControl::cmd_pbsz(const std::string&argument)
{
  if(!encrypted()){
    make_response("503","Cannot protect an unencrypted stream");
    return;
  }
  if(argument != "0"){
    make_response("501","PBSZ=0");
    return;
  }
  make_response("200","RFC 2228 - what a waste of time");
}


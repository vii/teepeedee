#include <sstream>

#include <cstring>
#include <cctype>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>

#include <user.hh>
#include <iocontextxfer.hh>
#include <filelister.hh>
#include <confexception.hh>
#include <xfertable.hh>
#include <path.hh>

#include "httpcontrol.hh"
#include "httplist.hh"



HTTPControl::HTTPControl(int fd, const ConfTree&c):
  _conf(c),
  _xfer(0)
{
  set_fd(fd);
  reset_req();
}

void
HTTPControl::hangup(XferTable&xt)
{
  if(_xfer)
    _xfer->hangup(xt);

  IOContextResponder::hangup(xt);
}


bool
HTTPControl::persistant()
{
  if(closing())
    return false;
  if(!strcasecmp(_req_headers["connection"].c_str(),"keep-alive"))
    return true;
  if(_req_ver_major != 1 || _req_ver_minor != 1)
    return false;
  if(!strcasecmp(_req_headers["connection"].c_str(),"close"))
    return false;
  
  return true;
}

static
std::string base64_decode(const std::string&coded)
{
  // Base64 is in RFC 1521
  /* python code to generate table
import base64

table = [-1] * 128

for i in range(64):
    table[ord(base64.encodestring(chr(0)+chr(0)+chr(i))[3])] = i

table[ord('=')] = 0
    
for i in range(128):
    if i & 7 == 0:
        print "\n\t",
    print "%2d,"%table[i],
  */
  static const signed char xlate[] = 
    {
        -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, 62, -1, -1, -1, 63, 
        52, 53, 54, 55, 56, 57, 58, 59, 
        60, 61, -1, -1, -1,  0, -1, -1, 
        -1,  0,  1,  2,  3,  4,  5,  6, 
         7,  8,  9, 10, 11, 12, 13, 14, 
        15, 16, 17, 18, 19, 20, 21, 22, 
        23, 24, 25, -1, -1, -1, -1, -1, 
        -1, 26, 27, 28, 29, 30, 31, 32, 
        33, 34, 35, 36, 37, 38, 39, 40, 
        41, 42, 43, 44, 45, 46, 47, 48, 
        49, 50, 51, -1, -1, -1, -1, -1
    };
  
  std::string ret;
  signed char quad[4];
  const char*pos = coded.c_str();

  while(*pos){
    
    for(int i = 0; i < 4; ++i){
      if(!*pos)
	return std::string();
      
      quad[i] = xlate[*pos++ & 127];
      if(quad[i] == (signed char)-1)
	return std::string();
    }
    for(int i = 0; i <3; ++i)
      ret += (quad[i] << (2*i+2)) | (quad[i+1] >> (4-2*i));
  }
  
  return ret;
}

bool
HTTPControl::try_authenticate(User&user)
{
  using std::isspace; // damn gcc-2.95.2 vs glibc
  std::string auth = _req_headers["authorization"];
  if(strncasecmp(auth.c_str(),"Basic",5))
     return false;

  const char*base64 = auth.c_str()+5;

  if(!isspace(*base64))
    return false;
    
  while(isspace(*++base64));

  std::string unamepasswd = base64_decode(base64);
  for(unsigned i = 0;unamepasswd.c_str()[i];++i)
    if(unamepasswd[i]==':'){
      ConfTree users;
      _conf.get("users",users);
      if(!user.authenticate(unamepasswd.substr(0,i),unamepasswd.substr(i+1),users))
	throw User::AccessException();
      return true;
    }
  
  return false;
}
  
void
HTTPControl::authenticate(User&user)
{
  if(try_authenticate(user))
    return;
  
  if(!user.authenticate("default-user","",_conf))
    throw User::AccessException();
}

void
HTTPControl::parse_req_line(XferTable&xt,char*buf,size_t len)
{
  char*end = buf+len;
  char*start;
  for(start=buf;start<end;++start){
    *start = std::tolower(*start);
    if(*start == ' '){
      *start++ = 0;
      _req_method = buf;
      break;
    }
  }
  
  
  for(char*i=start;i<end;++i)
    if(*i == ' '){
      *i++ = 0;
      _req_uri.parse(start);
      start = i;
      break;
    }

  if(start == end){
    error_response("400","What's the magic word?");
    return;
  }

  if(std::strncmp(start,"HTTP/",5)){
    error_response("400","No version in request");
    return;
  }

  start += 5;
  char* res;
  _req_ver_major = std::strtoul(start,&res,10);
  if(res==start || *res != '.'){
    error_response("400","Bad request major version number format");
    return;
  }
  start = res+1;
  _req_ver_minor = std::strtoul(start,&res,10);
  if(res == start){
    error_response("400","Bad request minor version number format");
    return;
  }
}
  
void
HTTPControl::parse_header(XferTable&xt,char*buf,size_t len)
{
  using std::isspace; // damn gcc-2.95.2 vs glibc

  char*end = buf+len;
  char*start;
  std::string name;
  for(start=buf;start<end;++start) {
    *start = std::tolower(*start);
    if(*start == ':'){
      *start++ = 0;
      name = buf;
      break;
    }
  }

  while(start < end && isspace(*start))
    start++;

  for(char*last = end-1; last >= start && isspace(*last); --last)
    *last = 0;

  std::string val = start;
  finished_header_line(name,val);
}

void
HTTPControl::finished_req(XferTable&xt)
{
#define method(name) { #name, & HTTPControl::cmd_ ## name }
  struct cmd
  {
    typedef void (HTTPControl::*function_type)(XferTable&xt);
    const char*name;
    function_type function;
  };
  static const cmd cmd_table[] = 
    {
      method(put),
      method(options),
      method(delete),
      method(trace),
      method(get),
      method(head),
      { 0 , 0 }
    };
#undef method
  
  try{
    for(const cmd*i=cmd_table;i->name;++i)
      if(_req_method == i->name) {
	(this->*i->function)(xt);
	finished_queuing_response();
	return;
      }
  } catch (User::AccessException&){
    error_response("401","I'm sorry, I can't let you do that");
    return;
  } catch (ConfException&ce){
    error_response("500","Server misconfigured: " + std::string(ce.what()));
    return;
  }
  
  error_response("501","Server doesn't understand");
}

void
HTTPControl::error_response(const std::string& code,const std::string& str)
{
  status_line(code,str);
  response_header_line("Content-Type","text/html");

  std::string err;
  if(_conf.exists("error-"+code))
    _conf.get("error-"+code,err);
  else
    err = std::string("<head><title>")+str+"</title></head>\n"
	       "<body><h1>"+str+"</h1>\n"
      "<h2>Error "+code+"</h2></body>\n";

  response_header_line("Content-Length",err.length());
  response_header_end();
  
  add_response(err);

  finished_queuing_response();
}

void
HTTPControl::response_header_line(const std::string& name, unsigned long value)
{
  std::ostringstream s;
  s << value;
  response_header_line(name,s.str());
}

void
HTTPControl::finished_reading(XferTable&xt,char*buf,size_t len)
{
  if(!has_req_line()) {
    parse_req_line(xt,buf,len);
    return;
  }
  if(len == 0) {
    finished_req(xt);
    return;
  }
  parse_header(xt,buf,len);
}

void
HTTPControl::response_header_end()
{
  if(!persistant())
    response_header_line("Connection","close");
  else {
    if(_req_ver_major == 1&&_req_ver_minor ==0)
      response_header_line("Connection","Keep-Alive");
  }
  if(_conf.exists("realm")){
    std::string realm;
    _conf.get_line("realm",realm);
    response_header_line("WWW-Authenticate","Basic realm=\"" + realm + "\"");
  }
  
  add_response("\r\n");
}

void
HTTPControl::xfer_done(IOContextControlled*xfer,bool successful)
{
  if(xfer != _xfer){
    warnx("internal error in HTTPControl::xfer_done");
    return;
  }

  set_fd(_xfer->get_fd());
  _xfer->set_fd(-1);
  _xfer = 0;
}


HTTPControl::events_t
HTTPControl::get_events()
{
  if(_xfer)
    return POLLIN|POLLOUT;
  return IOContextResponder::get_events();
}

void
HTTPControl::start_xfer(XferTable&xt,IOContextControlled*xfer)
{
  _xfer = xfer;
}

bool
HTTPControl::io(const struct pollfd&pfd,XferTable&xt)
{
  if(_xfer && !response_ready()){
    _xfer->set_fd(get_fd());
    set_fd(-1);
    xt.add(_xfer);
    if(closing()) {
      _xfer->detach();
      _xfer = 0;
      hangup(xt);
    } else {
      set_timeout_interval(0);
    }
    return true;
  }
  return IOContextResponder::io(pfd,xt);
}


Path
HTTPControl::get_uri_path()const
{
  return Path(_req_uri.path());
}

void
HTTPControl::do_cmd_get(XferTable&xt,bool actually_send_data)
{
  User user;
  authenticate(user);

  User::Stat buf;
  Path p = get_uri_path();
  int fd = -1;
  FileLister*fl = 0;
  
  if(!user.stat(p,buf)){
    error_response("404","No file by that name");
    return;
  }
  if(buf.is_dir())
    fl = user.list(p);
  else
    if((fd = user.open(p,O_RDONLY))==-1){
      error_response("404","Unable to open file");
      return;
    }
  status_line("200","File ready to send");
  if(!buf.is_dir()){
    response_header_line("Content-Length",buf.size());
  } else {
    close_after_output(); //no content length so must close
  }
  response_header_end();

  if(actually_send_data){
    IOContextControlled*ioc;
    if(fl){
      try{
	ioc = new HTTPList(this,fl,user);
      } catch (...){
	delete fl;
	throw;
      }
    } else {
      try {
	ioc = new IOContextXfer(this,get_fd(),fd);
      } catch (...){
	::close(fd);
	throw;
      }
    }
    start_xfer(xt,ioc);
  } else {
    if(fl)
      delete fl;
    if(fd != -1) {
      if(::close(fd)){
	warn("close after HTTP HEAD");
      }
    }
  }
}


void
HTTPControl::cmd_put(XferTable&xt)
{
  User user;
  authenticate(user);
  Path p = get_uri_path();
  if(has_req_header("content-encoding")
     || has_req_header("content-range") ){
    error_response("501","Content-* headers not implemented for PUT");
    return;
  }

  int fd = user.open(p,O_WRONLY|O_CREAT);
  if(fd == -1){
    error_response("404","Cannot create file");
    return;
  }

  IOContextControlled*ioc;
  try{
    ioc=new IOContextXfer(this,fd,get_fd());
  } catch (...){
    ::close(fd);
    throw;
  }

  start_xfer(xt,ioc);
  // XXX should check if being created and return 201 if so
  
  status_line("204","File being written");
  response_header_end();
}

void
HTTPControl::cmd_options(XferTable&xt)
{
  if(has_req_body()){
    // hey this is not even def'd in the 1.1 spec (RFC2616)
    error_response("501","OPTIONS message body not implemented");
    return;
  }
    
  if(_req_uri.str() == "*") {
    status_line("200","OPTIONS * happily received");
    response_header_line("Allow","GET, HEAD, PUT, DELETE");
    response_header_end();
    return;
  }
  
  User user;
  authenticate(user);
  Path p = get_uri_path();

  status_line("200","OPTIONS happily received");
  if(user.may_readfile(p))
    response_header_line("Allow","GET, HEAD");
  if(user.may_writefile(p))
    response_header_line("Allow","PUT");
  if(user.may_deletefile(p))
    response_header_line("Allow","DELETE");
  response_header_end();
}

void
HTTPControl::cmd_delete(XferTable&xt)
{
  User user;
  authenticate(user);
  Path p = get_uri_path();

  if(!user.unlink(p)){
    error_response("404","File to delete absent");
    return;
  }
  status_line("204","File successfully destroyed!");
  response_header_end();
  return;
}

void
HTTPControl::cmd_trace(XferTable&xt)
{
  error_response("501","TRACE not implemented as it is too easy to DoS the server");
}


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
#include <path.hh>
#include <streamcontainer.hh>

#include "xferlimituser.hh"
#include "httpcontrol.hh"
#include "httplist.hh"



HTTPControl::HTTPControl(const Conf&c,StreamContainer&s,const std::string&proto):
  Control(c,s),
  _protocol(proto),
  _xfer(0)
{
  reset_req();
  config().get_timeout("timeout_prelogin",*this);
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
      std::string username = unamepasswd.substr(0,i);
      if(!user_login(user,username,unamepasswd.substr(i+1)))
	throw User::AccessException();
      return true;
    }
  
  return false;
}
  
void
HTTPControl::authenticate(User&user)
{
  if(try_authenticate(user)) {
    config().get_timeout("timeout_postlogin",*this);
    return;
  }
  
  if(!user_login_default(user))
    throw User::AccessException();
  
  config().get_timeout("timeout_postlogin",*this);
}

void
HTTPControl::parse_req_line(char*buf,size_t len)
{
  using std::tolower; // for NetBSD 1.6
  
  char*end = buf+len;
  char*start;
  for(start=buf;start<end;++start){
    *start = tolower(*start);
    if(*start == ' '){
      *start++ = 0;
      _req_method = buf;
      break;
    }
  }
  
  
  for(char*i=start;i<end;++i)
    if(*i == ' '){
      _req_uri_is_dir = (i<=start) ? 0 : ( *(i-1) == '/');
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
HTTPControl::parse_header(char*buf,size_t len)
{
  using std::isspace; // damn gcc-2.95.2 vs glibc
  using std::tolower; // for NetBSD 1.6

  char*end = buf+len;
  char*start;
  std::string name;
  for(start=buf;start<end;++start) {
    *start = tolower(*start);
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
HTTPControl::finished_req()
{
#define method(name) { #name, & HTTPControl::cmd_ ## name }
  struct cmd
  {
    typedef void (HTTPControl::*function_type)();
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
	(this->*i->function)();
	finished_queuing_response();
	return;
      }
  } catch (const User::AccessException&){
    error_response("401","I'm sorry, I can't let you do that");
    return;
  } catch (const LimitException&){
    error_response("503","Server overloaded");
    return;
  } catch (const ConfException&ce){
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
  if(config().exists("error-"+code))
    config().get("error-"+code,err);
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
HTTPControl::finished_reading(char*buf,size_t len)
{
  if(!has_req_line()) {
    parse_req_line(buf,len);
    return;
  }
  if(len == 0) {
    finished_req();
    return;
  }
  parse_header(buf,len);
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
  std::string realm = "teepeedee";
  if(config().exists("realm")){
    config().get_line("realm",realm);
  }
  response_header_line("WWW-Authenticate","Basic realm=\"" + realm + "\"");
  
  add_response("\r\n");
}

void
HTTPControl::xfer_done(IOContextControlled*xfer,bool successful)
{
  if(xfer != _xfer){
    warnx("internal error in HTTPControl::xfer_done");
    return;
  }
  _xfer = 0;
}


void
HTTPControl::start_xfer(IOContextControlled*xfer,Stream*st)
{
  _xfer = xfer;
  if(st)
    stream_container().add(st);
  config().get_timeout("timeout_xfer",*xfer);
}

Path
HTTPControl::get_uri_path()const
{
  return Path(_req_uri.path());
}

std::string
HTTPControl::mimetype(const Path&path)
{
  if(path.empty())
    return std::string();
  std::string last=*path.rbegin();
  std::string::size_type pos = last.rfind('.');
  if(pos == std::string::npos)
    return std::string();
  std::string ext = last.substr(pos+1);
  if(ext == "html")
    return "text/html";
  return std::string();
}

void
HTTPControl::start_file_xfer(Stream*fd,
			     const std::string&filename,
			     User&user,
			     XferLimit::Direction::type dir)
{
  IOContextXfer*ioc;
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
    ioc->limit(new XferLimitUser(user,dir,remotename(),_protocol,
				 filename));
  } catch (...){
    fd->release_consumer();
    delete ioc;
    delete fd;
    throw;
  }
  start_xfer(ioc,fd);
}


void
HTTPControl::do_get_file(const Path&file,User::Stat&buf,
			 User&user,bool actually_send_data)
{
  Stream*fd;
  if(!(fd = user.open(file,O_RDONLY))){
    error_response("404","Unable to open file");
    return;
  }
  status_line("200","File ready to send");
  std::string mt = mimetype(file);
  if(!mt.empty())
    response_header_line("Content-Type",mt);
  response_header_line("Content-Length",buf.size());
  response_header_end();

  if(actually_send_data){
    start_file_xfer(fd,file.str(),user,XferLimit::Direction::download);
  } else
    if(fd) {
      delete(fd);
    }
}

void
HTTPControl::moved_to_response(const std::string& path)
{
  std::string servername = _req_headers["host"];
  if(servername.empty())
    servername = _req_uri.authority();
  if(config().exists("canonical_server_name"))
    config().get_line("canonical_server_name",servername);
  if(servername.empty()){
    error_response("500","Don't know what my canonical server name is");
    return;
  }

  std::string replacement = _protocol + "://" + servername + "/" + path;
  std::string body = std::string("<head><title>To pastures new</title></head>\n"
				 "<body><h1>Moved far away</h1>\n"
				 "<P>Now at <A href=\"" + replacement + "\">" + replacement + "</A>.</body>\n");
  
  status_line("301","Totally moved far away");
  response_header_line("Location",replacement);
  response_header_line("Content-Length",body.size());
  response_header_line("Content-Type","text/html");

  response_header_end();

  add_response(body);
  
  finished_queuing_response();
}

void
HTTPControl::do_get_dirlisting(const Path&dir,User&user,bool actually_send_data)
{
  FileLister*fl = user.list(dir);
  status_line("200","File ready to send");
  response_header_line("Content-Type","text/html");
  close_after_output(); //no content length so must close
  response_header_end();
  
  if(actually_send_data){
    IOContextControlled*ioc;
    try{
      ioc = new HTTPList(this,fl,user);
    } catch (...){
      delete fl;
      throw;
    }
    start_xfer(ioc);
  }
}

void
HTTPControl::do_cmd_get(bool actually_send_data)
{
  User user;
  authenticate(user);

  User::Stat buf;
  Path p = get_uri_path();
  
  if(!user.stat(p,buf)){
    error_response("404","No file by that name");
    return;
  }
  if(buf.is_dir()) {
    if(!_req_uri_is_dir) {
      moved_to_response(p.str() + "/");
      return;
    }
    
    bool no_index_html = false;
    config().get("no_index_html",no_index_html);
    if(!no_index_html){
      Path index(p);
      index.push_back("index.html");
      if(user.stat(index,buf)) {
	do_get_file(index,buf,user,actually_send_data);
	return;
      }
    }
    bool no_dir_listing = false;
    config().get("no_dir_listing",no_dir_listing);
    if(!no_dir_listing){
      do_get_dirlisting(p,user,actually_send_data);
      return;
    } else {
      error_response("403","Directory listing forbidden");
      return;
    }
  }
  do_get_file(p,buf,user,actually_send_data);
}


void
HTTPControl::cmd_put()
{
  User user;
  authenticate(user);
  Path p = get_uri_path();
  if(has_req_header("content-encoding")
     || has_req_header("content-range") ){
    error_response("501","Content-* headers not implemented for PUT");
    return;
  }

  Stream*fd = user.open(p,O_WRONLY|O_CREAT);
  if(!fd){
    error_response("404","Cannot create file");
    return;
  }

  start_file_xfer(fd,p.str(),user,XferLimit::Direction::upload);

  // XXX should check if being created and return 201 if so
  
  status_line("204","File being written");
  response_header_end();
}

void
HTTPControl::cmd_options()
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
HTTPControl::cmd_delete()
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
HTTPControl::cmd_trace()
{
  error_response("501","TRACE not implemented as it is too easy to DoS the server");
}

    
void
HTTPControl::read_in_xfer(Stream&stream,size_t max)
{
    made_progress();
    try{
      _xfer->read(stream,max);
    } catch (IOContext::Destroy&d){
      delete_xfer(d.target());
    }
    if(!_xfer)
      read_in(stream,max);
}

void
HTTPControl::read_in(Stream&stream,size_t max)
{
  if(_xfer) {
    read_in_xfer(stream,max);
    return;
  }
  super::read_in(stream,max);

  if(_xfer)
    read_in_xfer(stream,max);
}

void
HTTPControl::write_out_xfer(Stream&stream,size_t max)
{
  made_progress();
  try{
    _xfer->write(stream,max);
  } catch (IOContext::Destroy&d){
    delete_xfer(d.target());
  }
  if(!_xfer)
    return write_out(stream,max);
}



void
HTTPControl::write_out(Stream&stream,size_t max)
{
  if(_xfer && !response_ready()){
    write_out_xfer(stream,max);
    return;
  }
  super::write_out(stream,max);
  if(_xfer && !response_ready()){
    write_out_xfer(stream,max);
  }
}

bool
HTTPControl::want_write(Stream&stream)
{
  if(!_xfer)return super::want_write(stream);
  bool ret;
  try{
    ret = _xfer->want_write(stream);
  } catch(IOContext::Destroy&d){
    delete_xfer(d.target());
  }
  if(!_xfer)
    return super::want_write(stream);
  return ret;
}

bool
HTTPControl::want_read(Stream&stream)
{
  if(!_xfer)return super::want_read(stream);
  bool ret;
  try{
    ret = _xfer->want_read(stream);
  } catch (IOContext::Destroy&d){
    delete_xfer(d.target());
  }
  if(!_xfer)
    return super::want_read(stream);
  return ret;
}

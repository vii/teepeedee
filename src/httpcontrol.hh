#ifndef _TEEPEEDEE_SRC_HTTPCONTROL_HH
#define _TEEPEEDEE_SRC_HTTPCONTROL_HH

#include <string>
#include <map>

#include <iocontroller.hh>
#include <iocontextresponder.hh>
#include <conftree.hh>
#include <uri.hh>

class IOContextControlled;
class User;
class Path;

class HTTPControl : public IOContextResponder, public IOController
{
  typedef IOContextResponder super;
  ConfTree _conf;
  std::string _req_method;
  URI _req_uri;
  unsigned _req_ver_major;
  unsigned _req_ver_minor;
  IOContextControlled* _xfer;
  
  typedef std::map<std::string,std::string> _req_headers_type;
  _req_headers_type _req_headers;

  bool
  has_req_header(const std::string&name)
  {
    return _req_headers.find(name) != _req_headers.end();
  }
  
  bool
  persistant()
    ;

  bool
  try_authenticate(User&user)
    ;
  
  
  void
  authenticate(User&u);
  
  void
  finished_queuing_response() // may be called multiple times
  {
    if(!persistant())
      close_after_output();
    reset_req();
  }

  bool
  has_req_body()
  {
    return has_req_header("content-length") || has_req_header("content-encoding");
  }
  
  unsigned max_req_headers()const
  {
    return 100;
  }
  
  bool has_req_line()const
  {
    return !_req_method.empty();
  }
  void
  parse_req_line(XferTable&xt,char*buf,size_t len)
    ;
  
  void
  parse_header(XferTable&xt,char*buf,size_t len)
    ;

  void
  finished_req(XferTable&xt)
    ;
  
  void
  finished_header_line(const std::string&name,const std::string&val)
  {
    if(_req_headers.size() > max_req_headers()){
      error_response("500","Too many headers");
      return;
    }
    if(_req_headers.find(name) != _req_headers.end())
      _req_headers[name] += ", " + val;
    else
      _req_headers[name] = val;
  }

  void
  reset_req()
  {
    _req_method = std::string();
    _req_headers.erase(_req_headers.begin(),_req_headers.end());
    _req_ver_minor = 0;
    _req_ver_major = 0;
    _conf.get_timeout("timeout_prelogin",*this);
  }

  void
  hangup(XferTable&xt)
    ;
  
  void
  error_response(const std::string& code,const std::string& str)
    ;

  void
  start_xfer(XferTable&xt,IOContextControlled*xfer);
  
  void
  status_line(const std::string& code,const std::string& str)
  {
    add_response("HTTP/1.1 " + code + " " + str + "\r\n");
  }
  void
  response_header_line(const std::string& name, const std::string& value)
  {
    add_response(name + ": " + value + "\r\n");
  }
  void
  response_header_line(const std::string& name, unsigned long value)
    ;
  void
  response_header_end()
    ;
  
  Path
  get_uri_path()const
    ;
  
  // XXX TODO announce continue

  void
  do_cmd_get(XferTable&xt,bool actually_send_data)
    ;
  void
  cmd_put(XferTable&xt)
    ;
  void
  cmd_options(XferTable&xt)
    ;
  void
  cmd_delete(XferTable&xt)
    ;
  void
  cmd_trace(XferTable&xt)
    ;
  
  void
  cmd_get(XferTable&xt)
  {
    do_cmd_get(xt,true);
  }
  void
  cmd_head(XferTable&xt)
  {
    do_cmd_get(xt,false);
  }
  
  
protected:
  void
  finished_reading(XferTable&xt,char*buf,size_t len)
    ;
  
public:
  HTTPControl(int fd, const ConfTree&c)
    ;
  
  events_t get_events();

  void
  xfer_done(IOContextControlled*xfer,bool successful)
    ;

  bool
  io(const struct pollfd&pfd,XferTable&xt);
  
  std::string
  desc()const
  {
    return "http request " + super::desc();
  }
};
#endif

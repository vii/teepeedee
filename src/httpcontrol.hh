#ifndef _TEEPEEDEE_SRC_HTTPCONTROL_HH
#define _TEEPEEDEE_SRC_HTTPCONTROL_HH

#include <string>
#include <map>

#include <conftree.hh>
#include <uri.hh>
#include <iocontextcontrolled.hh>

#include "user.hh"
#include "control.hh"

class User;
class Path;

class HTTPControl : public Control
{
  typedef Control super;
  std::string _req_method;
  URI _req_uri;
  bool _req_uri_is_dir;
  unsigned _req_ver_major;
  unsigned _req_ver_minor;
  std::string _protocol;
  IOContextControlled* _xfer;
  Stream*_xfer_stream;
  
  typedef std::map<std::string,std::string> _req_headers_type;
  _req_headers_type _req_headers;

  bool
  has_req_header(const std::string&name)
  {
    return _req_headers.find(name) != _req_headers.end();
  }

  std::string
  mimetype(const Path&path)
    ;
  
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
  parse_req_line(char*buf,size_t len)
    ;
  
  void
  parse_header(char*buf,size_t len)
    ;

  void
  finished_req()
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
    _req_headers.clear();
    _req_ver_minor = 0;
    _req_ver_major = 0;
    _req_uri_is_dir = false;
    config().get_timeout("timeout_prelogin",*this);
  }

  void
  error_response(const std::string& code,const std::string& str)
    ;

  void
  start_xfer(IOContextControlled*xfer,Stream*st=0);

  void
  start_file_xfer(Stream*fd,
		  const std::string&filename,
		  User&user,
		  XferLimit::Direction::type dir)
    ;
  

  
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
  moved_to_response(const std::string& path)
    ;
  
  void
  response_header_end()
    ;
  
  Path
  get_uri_path()const
    ;
  
  // XXX TODO announce continue

  void
  do_cmd_get(bool actually_send_data)
    ;
  void
  do_get_file(const Path&file,User::Stat&buf,User&user,bool actually_send_data)
    ;
  void
  do_get_dirlisting(const Path&dir,User&user,bool actually_send_data)
    ;
  void
  cmd_put()
    ;
  void
  cmd_options()
    ;
  void
  cmd_delete()
    ;
  void
  cmd_trace()
    ;
  
  void
  cmd_get()
  {
    do_cmd_get(true);
  }
  void
  cmd_head()
  {
    do_cmd_get(false);
  }

  void
  free_xfer()
  {
    if(_xfer_stream){
      _xfer_stream->release_consumer();
      delete _xfer_stream;
      _xfer_stream = 0;
    }
    if(_xfer){
      delete _xfer;
      _xfer=0;
    }
  }
  
protected:
  void
  finished_reading(char*buf,size_t len)
    ;

  void
  read_in(Stream&stream,size_t max)
    ;
  
  void
  write_out(Stream&stream,size_t max)
    ;
  
  void
  read_in_xfer(Stream&stream,size_t max)
    ;
  
  void
  write_out_xfer(Stream&stream,size_t max)
    ;
  
public:
  HTTPControl(const ConfTree&c,const std::string& proto="http")
    ;
  
  void
  xfer_done(IOContextControlled*xfer,bool successful)
    ;

  bool
  want_write(Stream&stream)
    ;

  bool
  want_read(Stream&stream)
    ;
  

  std::string
  desc()const
  {
    return  _protocol + " " + super::desc();
  }

  ~HTTPControl()
  {
    free_xfer();
  }
  
};
#endif

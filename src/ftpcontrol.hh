#ifndef _TEEPEEDEE_SRC_FTPCONTROL_HH
#define _TEEPEEDEE_SRC_FTPCONTROL_HH

#include <string>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <conf.hh>
#include <path.hh>
#include "control.hh"
#include "user.hh"


class FTPDataListener;
class IOContextControlled;
class StreamContainer;
class SSLStreamFactory;

class FTPControl : public Control
{
  typedef Control super;
  User _user;

  FTPDataListener*_data_listener;//for passive mode
  IOContextControlled*_data_xfer;
  SSLStreamFactory*_ssl_factory;

  // These values are things cached for the FTP protocol
  std::string _username; // not necessarily the logged in user's username
  Path _path; 
  sockaddr_in _port_addr;
  sockaddr_in _local_addr;
  off_t _restart_pos;
  Path _rename_from;
  bool _quitting;
  bool _turning_into_ssl;
  Stream*_unencrypted_stream;

  bool
  encrypted()const
  {
    return _unencrypted_stream;
  }
  
  struct ftp_cmd 
  {
    typedef void (FTPControl::*function_type)(const std::string&argument);
    const char*name;
    function_type function;
    const char*desc;
  };
  static const ftp_cmd ftp_cmd_table[];
  

  off_t take_restart_pos()
  {
    off_t tmp = _restart_pos;
    _restart_pos = 0;
    return tmp;
  }

  bool
  restart(Stream*fd)
    ;
  
  bool passive()const
  {
    return _data_listener != 0;
  }

  // return true if good login
  bool
  login(const std::string&password)
    ;
  
  void
  command(const std::string&name,const std::string& argument)
    ;
  
  bool
  passive_on(bool epsv=false)
    ;
  bool
  passive_off()
    ;
  bool
  active_on()
    ;

  void
  start_file_xfer(const Path&path,
			    XferLimit::Direction::type dir)
    ;
  
  void
  start_xfer(IOContextControlled*xfer,
	     Stream*fd=0)
    ;

  void
  lose_auth()
  {
    _user.deauthenticate();
    _username = std::string();
    _restart_pos = 0;
    _rename_from.clear();
    _path.clear();
  }

  void make_response_proto_not_supported()
  {
    make_response("522","Network protocol not supported, use (1)");
  }

  
  void
  make_response(const std::string&code,const std::string&str,char connector=' ')
    ;

  void
  make_response_multiline(const std::string&code,const std::string&multiline);
  

  void
  make_error(const std::string&str)
  {
    make_response("421",str);
    close_after_output();
  }

  bool
  xfer_pending()const
  {
    return _data_xfer;
  }

  class InappropriateCommandException
  {
  };

  class UnauthenticatedException:public InappropriateCommandException
  {
  };

  class XferPendingException: public InappropriateCommandException
  {
  };

  void
  assert_auth()
  {
    if(!_user.authenticated())
      throw UnauthenticatedException();
  }
  void
  assert_no_xfer()
  {
    if(xfer_pending())
      throw XferPendingException();
  }  

  void
  cmd_user(const std::string&argument)
  {
    _username = argument;
    make_response("331","What's the secret password?");
    
  }
  void
  cmd_pass(const std::string&argument)
    ;
  
  void
  cmd_stat(const std::string&argument)
  {
    assert_auth();
    if(argument.empty()) {
      make_response("504","STAT not implemented with argument");
      return;
    }
    make_response("211",xfer_pending() ? "Transfer progressing" : "No transfer");
  }

  void
  cmd_help(const std::string&argument)
    ;

  void
  cmd_feat(const std::string&argument)
    ;

  void
  cmd_mdtm(const std::string&argument)
    ;
  
  void
  cmd_size(const std::string&argument)
    ;

  void
  cmd_mlst(const std::string&argument)
    ;

  void
  cmd_mlsd(const std::string&argument)
    ;
  
  void
  cmd_rein(const std::string&argument)
  {
    lose_auth();
    make_response("220","Hit me one more time baby");
  }

  void
  cmd_quit(const std::string&argument)
  {
    lose_auth();
    make_response("221","Closing control connection");
    if(!xfer_pending())
      close_after_output();
    _quitting = true;
  }

  void
  cmd_noop(const std::string&argument)
  {
    make_response("200","I did nothing!");
  }

  void
  cmd_syst(const std::string&argument)
  {
    make_response("215","UNIX Type: L8");
  }

  void
  cmd_type(const std::string&argument)
  {
    assert_auth();
    if(argument == "I" || argument == "A")
      make_response("200","TYPE irrelevant, nothing is ever mangled");
    else
      make_response("504","Is there any point in implementing this TYPE?");
      
    
  }

  void
  cmd_mode(const std::string&argument)
  {
    assert_auth();
    if(argument == "S")
      make_response("200","MODE S is the only one supported");
    else
      make_response("504","Is there any point in implementing this MODE?");
  }

  void
  cmd_stru(const std::string&argument)
  {
    assert_auth();
    if(argument == "F")
      make_response("200","STRU F is the only one supported");
    else
      make_response("504","Is there any point in implementing this STRU?");
    
  }

  void
  cmd_pasv(const std::string&argument)
  {
    assert_auth();
    if(!passive_on()){
      make_response("502","Passive mode not available at the moment");
    }
  }
  

  void
  cmd_pwd(const std::string&argument)
  {
    assert_auth();
    make_response("257","\"/" + _path.str() + "\" is current directory.");
    
  }

  void
  cmd_abor(const std::string&argument)
    ;

  void
  cmd_cdup(const std::string&argument)
  {
    assert_auth();
    cmd_cwd("..");
  }

  void
  cmd_nlst(const std::string&argument)
  {
    do_cmd_list(argument,false);
  }
  void
  cmd_list(const std::string&argument)
  {
    do_cmd_list(argument,true);
  }

  void
  cmd_rnfr(const std::string&argument)
  {
    assert_auth();
    _rename_from = _path;
    _rename_from.chdir(argument);
    make_response("350","Where to boss?");
    
  }
  void
  cmd_rnto(const std::string&argument)
  {
    assert_auth();
    Path to(_path,argument);
    if(to.empty()||_rename_from.empty()) {
      make_response("553","Give me names, names");
      return;
    }
    if(!_user.rename(_rename_from,to))
      make_response("550","Cannot change name, try deed poll");
    else
      make_response("250","Aye aye, captain");
    
    _rename_from.clear();
  }
  
  void
  cmd_cwd(const std::string&argument)
    ;
  void
  cmd_dele(const std::string&argument)
    ;
  
  void
  cmd_epsv(const std::string&argument)
    ;
  void
  cmd_eprt(const std::string&argument)
    ;
  void
  cmd_auth(const std::string&argument)
    ;
  void
  cmd_pbsz(const std::string&argument)
    ;
  
  void
  cmd_port(const std::string&argument)
    ;
  void
  cmd_retr(const std::string&argument)
    ;

  void
  cmd_stor(const std::string&argument)
    ;

  void
  cmd_rest(const std::string&argument)
    ;

  void
  cmd_mkd(const std::string&argument)
  {
    assert_auth();
    if(!_user.mkdir(Path(_path,argument)))
      make_response("550","Directory impossible");
    else
      make_response("257","Directory made extremely well");
  }
  
  void
  cmd_rmd(const std::string&argument)
  {
    assert_auth();
    if(!_user.rmdir(Path(_path,argument)))
      make_response("550","Utterly incapable of removing directory, is it empty?");
    else
      make_response("250","Directory soundly deleted");
  }
  
  void
  do_cmd_list(const std::string&argument,bool detailed)
    ;
protected:

  void
  finished_reading(char*buf,size_t len)
    ;
  
  
  void
  input_line_too_long()
  {
    make_error("Input line too long for my attention span");
  }

  bool
  stream_hungup(Stream&stream)
    ;
  
  void
  timedout(Stream&stream)
  {
    if(!response_ready()){
      make_error("Got bored waiting");
      return;
    }
    return super::timedout(stream);
  }
protected:
  void
  read_in(Stream&stream,size_t max)
    ;
public:
  FTPControl(const Conf&conf,StreamContainer&sc,SSLStreamFactory*ssf=0)
    ;

  void
  remote_stream(const Stream&stream)
    ;
  
  
  void
  xfer_done(IOContextControlled*xfer,bool successful);
  ;
  
  std::string
  desc()const
  {
    return "ftp " + super::desc();
  }
  
  ~FTPControl()
    ;
};

#endif

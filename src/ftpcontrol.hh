#ifndef _TEEPEEDEE_SRC_FTPCONTROL_HH
#define _TEEPEEDEE_SRC_FTPCONTROL_HH

#include <string>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <conftree.hh>
#include <iocontextresponder.hh>
#include <iocontroller.hh>
#include <path.hh>
#include "user.hh"



class FTPDataListener;
class IOContextControlled;

class FTPControl : public IOContextResponder,public IOController
{
  typedef IOContextResponder super;
  ConfTree _conf;
  User _user;
  bool _authenticated;

  FTPDataListener*_data_listener;//for passive mode
  IOContextControlled*_data_xfer;


  // These values are things cached for the FTP protocol
  std::string _username; // not necessarily the logged in user's username
  Path _path; 
  sockaddr_in _port_addr;
  off_t _restart_pos;
  Path _rename_from;
  bool _quitting;


  struct ftp_cmd 
  {
    typedef void (FTPControl::*function_type)(XferTable&xt,const std::string&argument);
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
  restart(int fd)
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
  command(const std::string&name,const std::string& argument,XferTable&xt)
    ;
  
  bool
  passive_on(XferTable&xt)
    ;
  bool
  passive_off(XferTable&xt)
    ;

  void
  start_xfer(XferTable&xt,IOContextControlled*xfer)
    ;
  
  
  void
  lose_auth()
  {
    _authenticated = false;
    _username = std::string();
    _restart_pos = 0;
    _rename_from.clear();
    _path.clear();
  }
  
  void
  make_response(const std::string&code,const std::string&str)
    ;

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
    if(!_authenticated)
      throw UnauthenticatedException();
  }
  void
  assert_no_xfer()
  {
    if(xfer_pending())
      throw XferPendingException();
  }  

  void
  cmd_user(XferTable&xt,const std::string&argument)
  {
    _username = argument;
    make_response("331","What's the secret password?");
    
  }
  void
  cmd_pass(XferTable&xt,const std::string&argument)
  {
    if(!login(argument)){
      make_response("530","You are not everything you claim to be");
    } else
      make_response("230","Please come in");
    
  }

  void
  cmd_stat(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(argument.empty()) {
      make_response("504","STAT not implemented with argument");
      return;
    }
    make_response("211",xfer_pending() ? "Transfer progressing" : "No transfer");
  }

  void
  cmd_help(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_feat(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_mdtm(XferTable&xt,const std::string&argument)
    ;
  
  void
  cmd_size(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_mlst(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_mlsd(XferTable&xt,const std::string&argument)
    ;
  
  void
  cmd_rein(XferTable&xt,const std::string&argument)
  {
    lose_auth();
    make_response("220","Hit me one more time baby");
  }

  void
  cmd_quit(XferTable&xt,const std::string&argument)
  {
    lose_auth();
    make_response("221","Closing control connection");
    if(!xfer_pending())
      close_after_output();
    _quitting = true;
  }

  void
  cmd_noop(XferTable&xt,const std::string&argument)
  {
    make_response("200","I did nothing!");
  }

  void
  cmd_syst(XferTable&xt,const std::string&argument)
  {
    make_response("215","UNIX Type: L8");
  }

  void
  cmd_type(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(argument == "I" || argument == "A")
      make_response("200","TYPE irrelevant, nothing is ever mangled");
    else
      make_response("504","Is there any point in implementing this TYPE?");
      
    
  }

  void
  cmd_mode(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(argument == "S")
      make_response("200","MODE S is the only one supported");
    else
      make_response("504","Is there any point in implementing this MODE?");
    
  }

  void
  cmd_stru(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(argument == "F")
      make_response("200","STRU F is the only one supported");
    else
      make_response("504","Is there any point in implementing this STRU?");
    
  }

  void
  cmd_pasv(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(!passive_on(xt)){
      make_response("502","Passive mode not available at the moment");
    }
  }
  

  void
  cmd_pwd(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    make_response("257","\"/" + _path.str() + "\" is current directory.");
    
  }

  void
  cmd_abor(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_cdup(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    return cmd_cwd(xt,"..");
  }

  void
  cmd_nlst(XferTable&xt,const std::string&argument)
  {
    do_cmd_list(xt,argument,false);
  }
  void
  cmd_list(XferTable&xt,const std::string&argument)
  {
    do_cmd_list(xt,argument,true);
  }

  void
  cmd_rnfr(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    _rename_from = _path;
    _rename_from.chdir(argument);
    make_response("350","Where to boss?");
    
  }
  void
  cmd_rnto(XferTable&xt,const std::string&argument)
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
  cmd_cwd(XferTable&xt,const std::string&argument)
    ;
  void
  cmd_dele(XferTable&xt,const std::string&argument)
    ;
  
  void
  cmd_port(XferTable&xt,const std::string&argument)
    ;
  void
  cmd_retr(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_stor(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_rest(XferTable&xt,const std::string&argument)
    ;

  void
  cmd_mkd(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(!_user.mkdir(Path(_path,argument)))
      make_response("550","Directory impossible");
    else
      make_response("257","Directory made extremely well");
  }
  
  void
  cmd_rmd(XferTable&xt,const std::string&argument)
  {
    assert_auth();
    if(!_user.rmdir(Path(_path,argument)))
      make_response("550","Utterly incapable of removing directory, is it empty?");
    else
      make_response("250","Directory soundly deleted");
  }
  
  void
  do_cmd_list(XferTable&xt,const std::string&argument,bool detailed)
    ;
protected:

  void
  finished_reading(XferTable&xt,char*buf,size_t len)
    ;
  
  
  void
  input_line_too_long(XferTable&xt)
  {
    make_error("Input line too long for my attention span");
  }
  
public:
  FTPControl(int fd,const ConfTree&conf)
    ;
  
  bool
  timedout(XferTable&xt)
  {
    if(!response_ready()){
      make_error("Got bored waiting");
      return true;
    }
    return IOContextWriter::timedout(xt);
  }
  
  
  void
  xfer_done(IOContextControlled*xfer,bool successful);
  ;
  
  void
  hangup(class XferTable&xt)
    ;
  
  std::string
  desc()const
  {
    return "ftp control " + super::desc();
  }

  events_t get_events();
  
  ~FTPControl()
    ;
};

#endif

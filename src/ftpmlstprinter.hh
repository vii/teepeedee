#ifndef _TEEPEEDEE_SRC_FTPMLSTPRINTER_HH
#define _TEEPEEDEE_SRC_FTPMLSTPRINTER_HH

#include <sstream>
#include <ctime>

#include <user.hh>
#include <path.hh>
class FTPMlstPrinter
{
  User _user;
public:
  
  FTPMlstPrinter(const User&u):_user(u)
  {
  }
  static
  std::string
  mtime(std::time_t mtime)
  {
    struct tm tmbuf;
    struct tm *split_mtime = gmtime_r(&mtime,&tmbuf);
    char datebuf[64];
  
    if(!std::strftime(datebuf,sizeof datebuf,"%Y%m%d%H%M%S",split_mtime)){
      warn("strftime");
      strcpy(datebuf,"00000000000000");
    }
    datebuf[sizeof(datebuf)-1]=0;
    return datebuf;
  }
  
  std::string
  line(const Path&p)
  {
    return facts(p) + "; " + p.str();
  }
  
  std::string
  facts(const Path&p)
  {
    try {
      User::Stat buf;
      std::ostringstream s;
      s << "Perm=";
      if(!_user.stat(p,buf))
	return s.str();
      if(buf.may_read()) {
	if(buf.is_dir())
	  s << 'e';
	s << 'r';
      }
      if(buf.may_write()){
	s << 'w';
      }
      s <<  ";Size=" << buf.size();
      if(buf.is_file())
	s << ";Type=file";
      if(buf.is_dir())
	s << ";Type=dir";
      s << ";Modified=" << mtime(buf.mtime());
      return s.str();
    } catch (User::AccessException&ue){
      return "Perm=";
    }
  }
  
};

#endif

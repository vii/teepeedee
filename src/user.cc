#include <sstream>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <err.h>
#include <sys/stat.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif


#include <path.hh>
#include <filelisterdir.hh>
#include <filelisterfile.hh>
#include <filelisternone.hh>
#include <limitexception.hh>
#include <streamfd.hh>

#include "user.hh"

static const unsigned max_username_length = 200;



Stream*
User::open(const Path&fname,int flags,mode_t mode)
{
  int fd = open_fd(fname,flags,mode);
  try{
    return new StreamFD(fd);
  } catch(...){
    close(fd);
    throw;
  }
}

int
User::open_fd(const Path&fname,int flags,mode_t mode)
{
  bool exist = exists(fname);
  if((flags&O_ACCMODE) == O_WRONLY) {
    if((exist && !may_writefile(fname))
       // obvious race!
       || (!exist && !may_createfile(fname)))
      throw AccessException();
    off_t max = xfer_limit(XferLimit::Direction::upload);
    if(max){
      off_t current = xfer_total(XferLimit::Direction::upload);
      if(current >= max)
	throw XferLimitException();
    }
  } else if (!exist)
    throw AccessException();
  
  if(exist && (flags&O_ACCMODE) == O_RDONLY)
    if(!may_readfile(fname))
      throw AccessException();

  int fd = ::open(get_path(fname).c_str(),flags,mode);
  if(fd == -1)
    throw UnixException("open");

  Stat buf;
  if(!buf.fstat(fd)){
    close(fd);
    throw AccessException();
  }
  if(((flags&O_ACCMODE) == O_WRONLY && !buf.may_write())
     || ((flags&O_ACCMODE) == O_RDONLY && !buf.may_read())){
    close(fd);
    throw AccessException();
  }
  return fd;
}

FileLister*
User::list_one(const Path&path)
{
  User::Stat buf;
  if(!stat(path,buf))
    return new FileListerNone();
  return new FileListerFile(path);
}

FileLister*
User::listdir(const Path&path)
{
  if(!may_listdir(path))
    throw AccessException();
    
  FileListerDir* fld = new FileListerDir(get_path(path),path);
  int fd = fld->get_fd();
  if(fd != -1) {
    Stat buf;
    if(!buf.fstat(fd) || !check_access(buf,access::read,access::dir)){
      delete fld;
      throw AccessException();
    }
  }
  
  return fld;
}


FileLister*
User::list(const Path&path)
{
  User::Stat buf;
  if(!stat(path,buf))
    return new FileListerNone();
  
  if(buf.is_dir()){
    return listdir(path);
  }
  return list_one(path);
}

bool
User::rename(const Path&from,const Path&to)
{
  if(!may_delete(from) || !may_create(to))
    throw AccessException();

  return ::rename(get_path(from).c_str(),get_path(to).c_str()) != -1;
}

bool User::may_stat(const Path&path)
{
  Path parent(path);
  if(!parent.empty())
    parent.pop_back();
  return may_listdir(parent);
}

bool
User::authenticate(const std::string&username,
		   const std::string&password,
		   const ConfTree&usersdb)
{
  if(!verify_password(username,password,usersdb))
    return false;
  if(!_conf.book_increment("logins",1)){
    throw LimitException();
  }
  _name = username;
  if(!_conf.exists(XferLimit::Direction::to_string(XferLimit::Direction::download) + "_bytes"))
    _accounting_download = false;
  if(!_conf.exists(XferLimit::Direction::to_string(XferLimit::Direction::upload) + "_bytes"))
    _accounting_upload = false;
  _authenticated = true;
  return true;
}

void
User::deauthenticate()
{
  if(_authenticated){
    _authenticated = false;
    _conf.book_decrement("logins",1);
  }
}

bool
User::verify_password(const std::string&username,
		   const std::string&password,
		   const ConfTree&usersdb)
{
  using std::isalnum; // damn gcc-2.95.2 vs glibc
  
  // verify username is not dodgy
  for(std::string::const_iterator i = username.begin();
      i != username.end(); ++i){
    if(!isalnum(*i) && *i != '_' && *i != '-')
      return false;

    // and let's make double extra sure in case some clown sets a locale
    if(*i == '.' || *i == '/')
      return false;
  }

  if(username.empty())
    return false;
  if(username.size() >= max_username_length)
    return false;

  if(!usersdb.exists(username))
    return false;

  try{
    usersdb.get(username,_conf);

    bool any_password_ok = false;
    _conf.get("any_password_ok",any_password_ok);
    if(any_password_ok)
      return true;

    std::string real_password;
    if(_conf.exists("password_plain")) {
      _conf.get_line("password_plain",real_password);

      // have to use strcmp as one of these may not be nul-terminated
      if(!std::strcmp(real_password.c_str(),password.c_str()))
	return true;
      return false;
    }
    
    _conf.get_line("password",real_password);

    char*c=crypt(password.c_str(),real_password.c_str());
    if(!std::strcmp(c,real_password.c_str()))
      return true;    
    return false;
  }catch(std::exception&e){
    warnx("error trying to authenticate user: %s",e.what());
  }
  return false;
}

bool
User::check_access(const Path&path,access::type type,access::node_type node_type)
{
  Stat buf;

  if(!buf.stat(get_path(path)))
    return false;

  return check_access(buf,type,node_type);
}


bool
User::check_access(const Stat&buf,access::type type,access::node_type node_type){
  if(type == access::read){
    if(!buf.may_read())
      return false;
  }
  if(type == access::write){
    if(!buf.may_write())
      return false;
  }
  if(node_type == access::file){
    if(!buf.is_file())
      return false;
  }
  if(node_type == access::dir){
    if(!buf.is_dir())
      return false;
  }
  
  return true;
}

bool User::may_listdir(const Path&path)
{
  return access_dir(path,access::read);
}

bool User::may_chdir(const Path&path)
{
  return access_dir(path,access::read);
}

bool User::may_readfile(const Path&path)
{
  return access_file(path,access::read);
}

bool User::may_writefile(const Path&path)
{
  if(exists(path)){
    return access_file(path,access::write);
  } else {
    return may_createfile(path);
  }
}

bool User::may_create(const Path&path)
{
  if(path.empty())
    return false;
  
  Path p(path);
  p.pop_back();
  return access_dir(p,access::write);
}

bool User::stat(const Path&path,Stat&buf)
{
  if(!may_stat(path))
    throw AccessException();
  return buf.stat(get_path(path));
}

std::string User::get_path(const Path&path) {
  return get_root() + "/" + path.str();
}

bool User::unlink(const Path&path)
{
  if(!may_deletefile(path))
    throw AccessException();

  return !::unlink(get_path(path).c_str());
}

bool User::may_delete(const Path&path) {
  Path p(path);
  if(!check_access(p,access::write,access::any))
    return false;
  p.pop_back();
  return access_dir(p,access::write);
}

bool
User::mkdir(const Path&path)
{
  if(!may_createdir(path))
    throw AccessException();
  return !::mkdir(get_path(path).c_str(),0777);
}

bool
User::rmdir(const Path&path)
{
  if(!may_deletedir(path))
    throw AccessException();
  return !::rmdir(get_path(path).c_str());
}

std::string
User::message_login()
{
  std::string welcome;
  _conf.get_if_exists("msg_welcome",welcome);
  
  return message_limits() + welcome;
}

void
User::do_print_limit(std::ostream&limits,XferLimit::Direction::type dir)
{
  off_t max=0, current=0;
  max = xfer_limit(dir);
  if(!max ||
     (dir == XferLimit::Direction::download && !_accounting_download)
     || (dir == XferLimit::Direction::upload && !_accounting_upload))
    limits << "There is no " << XferLimit::Direction::to_string(dir) << " limit.";
  else {
    limits << "The " << XferLimit::Direction::to_string(dir) << " limit was " << max << " bytes.";
  }
  try{
    current = xfer_total(dir);
    //    limits << " Already " << current << " bytes have been " << XferLimit::Direction::to_string(dir) << "ed.";
    if(max){
      if(current >= max){
	limits << " You may not " << XferLimit::Direction::to_string(dir) << " anything.";
      } else {
	limits << " You may now " << XferLimit::Direction::to_string(dir) << " " << max - current << " bytes.";
      }
    }
  }catch(ConfException&){
  }

  limits << '\n';
}
  

std::string
User::message_limits()
{
  std::ostringstream limits;
  do_print_limit(limits,XferLimit::Direction::download);
  ratio_t rat = upload_ratio();
  if(rat && _accounting_upload)
    limits << "For every byte you upload you may download " << rat << " more bytes.\n";
  do_print_limit(limits,XferLimit::Direction::upload);
  
  {
    int max=0,cur=0;
    _conf.get_if_exists("logins_max",max);
    _conf.get_atomically_if_exists("logins",cur);
    limits << "There are " << cur << " users in your class logged in.";
    if(max) {
      limits << " The limit is " << max << " logins.";
    }
    limits << '\n';
  }

  return limits.str();
}

std::string
User::message_directory(const Path&path)
{
  std::string ret;
  try{
    Path index(path);
    index.push_back(".message");
    int fd=open_fd(index,O_RDONLY);
    char buf[8000];
    std::FILE*stream=fdopen(fd,"r");
    size_t s=fread(buf,1,sizeof(buf)-1,stream);
    if(ferror(stream))
      warnx("error reading directory index \"%s\"",get_path(index).c_str());
    fclose(stream);
    buf[s] = 0;
    ret = buf;
  }
  catch(...){
  }
  return ret;
}


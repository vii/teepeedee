#include <cctype>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <err.h>
#include <sys/stat.h>

#include "config.h"

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif


#include <path.hh>
#include <filelisterdir.hh>
#include <filelisterfile.hh>
#include <filelisternone.hh>

#include "user.hh"

static const unsigned max_username_length = 200;

int
User::open(const Path&fname,int flags,mode_t mode)
{
  bool exist = exists(fname);
  if((flags&O_ACCMODE) == O_WRONLY) {
    if((exist && !may_writefile(fname))
       // obvious race!
       || (!exist && !may_createfile(fname)))
      throw AccessException();
  } else if (!exist)
    throw AccessException();
  
  if(exist && (flags&O_ACCMODE) == O_RDONLY)
    if(!may_readfile(fname))
      throw AccessException();

  int fd = ::open(get_path(fname).c_str(),flags,mode);
  if(fd == -1)
    return fd;

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

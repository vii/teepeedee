#ifndef _TEEPEEDEE_SRC_USER_HH
#define _TEEPEEDEE_SRC_USER_HH

#include <string>
#include <exception>
#include <ctime>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <xferlimit.hh>
#include <conftree.hh>
#include <xferlimitexception.hh>
class Path;
class FileLister;
class Stream;

class User
{
  typedef float ratio_t;
  
  ConfTree _conf;
  std::string _name;

  // optimizations
  bool _accounting_upload;
  bool _accounting_download;

  // this field only signals whether this particular authentication
  // object was explicitly initialised with user_login and should decrement
  // the number of logins of the user when destroyed
  bool _authenticated;

  int open_fd(const Path&fname,int flags,mode_t mode=0664)
    ;
  
  std::string
  get_root()const
  {
    std::string root;
    _conf.get_fsname("homedir",root);
    return root;
  }

  std::string get_path(const Path&path)
    ;
  
  struct access
  {
    enum type {
      none,
      read,
      write
    };
    enum node_type {
      any,
      file,
      dir
    };
  private:
    access()
    {
    }
  };

  bool
  check_access(const Path&path,access::type type,access::node_type node_type)
    ;
  
  bool
  access_dir(const Path&path,access::type type)
  {
    return check_access(path,type,access::dir);
  }
  bool
  access_file(const Path&path,access::type type)
  {
    return check_access(path,type,access::file);
  }
  bool
  exists(const Path&path)
  {
    return check_access(path,access::none,access::any);
  }

  bool may_deletedir(const Path&path)
  {
    return may_delete(path);
  }
  bool may_delete(const Path&path)
    ;
  
  bool may_listdir(const Path&path)
    ;
  bool may_stat(const Path&path)
    ;
  
  bool may_create(const Path&path)
    ;
  bool may_createfile(const Path&path)
  {
    return may_create(path);
  }
  
  bool may_createdir(const Path&path)
  {
    return may_create(path);
  }

  ratio_t
  upload_ratio()
  {
    ratio_t rat = 0;
    _conf.get_if_exists("upload_ratio",rat);
    return rat;
  }
public:
  class Stat;
private:
  bool
  check_access(const Stat&buf,access::type type,access::node_type node_type)
  ;
  
public:
  User():
	 _accounting_upload(true),
	 _accounting_download(true),
	 _authenticated(false)
  {
  }
  User(const User&u):_conf(u._conf),
		     _accounting_upload(u._accounting_upload),
		     _accounting_download(u._accounting_download),
		     _authenticated(false)
  {
  }

  std::string
  username()const
  {
    return _name;
  }

  bool authenticated()const
  {
    return _authenticated;
  }
  
  
  // flags may not include O_RDWR
  Stream* open(const Path&fname,int flags,mode_t mode=0664)
    ;

  class AccessException
  {
  public:
    const char*what()const throw()
    {
      return "permission denied";
    }
  };
  
  bool stat(const Path&path,Stat&buf)
    ;
  bool unlink(const Path&path);

  bool mkdir(const Path&path);
  bool rmdir(const Path&path);
  
  bool rename(const Path&from,const Path&to)
    ;

  bool authenticate(const std::string&username,
		    const std::string&password,
		    const ConfTree&usersdb)
    ;
  void deauthenticate()
    ;
  ~User()
  {
    deauthenticate();
  }
  
  off_t
  xfer_total(XferLimit::Direction::type dir,bool do_lock=true)
  {
    std::string name = XferLimit::Direction::to_string(dir)+"_bytes";
    off_t current = 0;
    _conf.get_atomically_if_exists(name,current);

    return current;
  }
  
  off_t
  xfer_limit(XferLimit::Direction::type dir)
  {
    std::string name = XferLimit::Direction::to_string(dir)+"_bytes_max";
    off_t ret = 0;
    _conf.get_if_exists(name,ret);
    if(dir==XferLimit::Direction::download){
      ratio_t ratio = upload_ratio();
      if(!ratio)
	return ret;
      ret += off_t(ratio * xfer_total(XferLimit::Direction::upload));
    }
    return ret;
  }    
  
  bool
  book_xfer(off_t bytes,XferLimit::Direction::type dir)
  {
    if(dir == XferLimit::Direction::download && !_accounting_download)
      return true;
    if(dir == XferLimit::Direction::upload && !_accounting_upload)
      return true;
    return _conf.book_increment(XferLimit::Direction::to_string(dir)+"_bytes",xfer_limit(dir),bytes);
  }
  
  std::string
  message_login()
    ;
  std::string
  message_limits()
    ;
  std::string
  message_directory(const Path&path)
    ;

  // returned filelister should be deleted
  FileLister*
  list(const Path&path)
    ;

  FileLister*
  list_one(const Path&path)
    ;

  FileLister*
  listdir(const Path&path)
    ;

  bool may_chdir(const Path&path)
    ;
  bool may_deletefile(const Path&path)
  {
    return may_delete(path);
  }
  bool may_readfile(const Path&path)
    ;
  
  bool may_writefile(const Path&path)
    ;
  
  class Stat
  {
    struct stat _stat;
    
  protected:
    friend class User;

    bool stat(const std::string&pathname)
    {
      return !::stat(pathname.c_str(),&_stat);
    }
    bool fstat(int fd)
    {
      return !::fstat(fd,&_stat);
    }

  public:
    bool
    is_dir()const
    {
      return S_ISDIR(_stat.st_mode);
    }

    bool
    is_file()const
    {
      return S_ISREG(_stat.st_mode);
    }

    bool
    may_read()const
    {
      if(! (S_IROTH & _stat.st_mode))
	return false;
      if(is_dir())
	if(! (S_IXOTH & _stat.st_mode))
	  return false;
      return true;
    }
    bool
    may_write()const
    {
      if(getuid() != _stat.st_uid)
	return false;
      if(getgid() != _stat.st_gid)
	return false;
      if(! (S_IWUSR & _stat.st_mode))
	return false;
      if(! (S_IWGRP & _stat.st_mode))
	return false;
      return true;
    }
    
    
    std::string rwx_string()const
    {
      std::string ret;
      if(is_dir())
	ret += "d";
      else
	ret += "-";
      std::string perms;
      perms += may_read() ? "r" : "-";
      perms += may_write() ? "w" : "-";
      perms += S_IXOTH &  _stat.st_mode ? "x" : "-";
      ret += perms + perms + perms;
      return ret;
    }
    unsigned nlink()const
    {
      return _stat.st_nlink;
    }
    std::string username()const
    {
      return "ftp";
    }
    std::string groupname()const
    {
      return "ftp";
    }
    off_t
    size()const
    {
      return _stat.st_size;
    }
    std::time_t
    mtime()const
    {
      return _stat.st_mtime;
    }
  };
private:
  void
  do_print_limit(std::ostream&limits,XferLimit::Direction::type dir)
    ;
  bool verify_password(const std::string&username,
		    const std::string&password,
		    const ConfTree&usersdb)
    ;
};

#endif

#ifndef _TEEPEEDEE_FTPLISTLONG_HH
#define _TEEPEEDEE_FTPLISTLONG_HH

#include "ftplist.hh"
class User;

class FTPListLong:public FTPList
{
  User _user;
protected:
  void
  prepare_entry()
    ;
  
public:
  FTPListLong(IOController*fc,const User&u,FileLister*fl):
    FTPList(fc,fl),_user(u)
  {
    set_entry("total 0\r\n"); // don't bother to figure out what it should be
  }

  std::string
  desc()const
  {
    return "Long " + FTPList::desc();
  }
};

#endif

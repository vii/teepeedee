#ifndef _TEEPEEDEE_SRC_HTTPLIST_HH
#define _TEEPEEDEE_SRC_HTTPLIST_HH

#include <iocontextlist.hh>
#include <user.hh>

class HTTPList:public IOContextList
{
  bool _finished;
  User _user;
  
protected:
  void
  finished_listing(XferTable&xt);
  void
  prepare_entry()
    ;
public:
  HTTPList(IOController*fc,FileLister*fl,const User&u)
    ;
    
  std::string
  desc()const
  {
    return "HTTP " + IOContextList::desc();
  }
  
};


#endif

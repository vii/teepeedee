#ifndef _TEEPEEDEE_LIB_XFERLIMITUSER_HH
#define _TEEPEEDEE_LIB_XFERLIMITUSER_HH

#include <xferlimit.hh>
#include "user.hh"

class XferLimitUser : public XferLimit {
  User _user;
protected:
  virtual
  bool // true if allowed to xfer that many bytes
  book_xfer(off_t bytes,Direction::type dir)
  {
    return _user.book_xfer(bytes,dir);
  };
public:
  XferLimitUser(const User&u,
		Direction::type dir,
		const std::string& remotename,
		const std::string& service,
		const std::string&resource
		):XferLimit(dir,remotename,service,resource,u.username()),
		  _user(u)
  {
  }
    
};


#endif

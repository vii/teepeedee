#ifndef _TEEPEEDEE_FTPLIST_HH
#define _TEEPEEDEE_FTPLIST_HH

#include <string>

#include <iocontextlist.hh>

class FTPList:public IOContextList
{
protected:
  void prepare_entry()
  {
    set_entry(entry_name()+"\r\n");
  }
  
public:
  FTPList(IOController*fc,FileLister*fl):
    IOContextList(fc,fl)
  {
  }

  std::string
  desc()const
  {
    return "FTP " + IOContextList::desc();
  }
};

#endif

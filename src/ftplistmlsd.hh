#ifndef _TEEPEEDEE_FTPLISTMLSD_HH
#define _TEEPEEDEE_FTPLISTMLSD_HH

#include <ftplist.hh>
#include <ftpmlstprinter.hh>

class FTPListMlsd:public FTPList
{
  FTPMlstPrinter _printer;
protected:
  void prepare_entry()
  {
    set_entry(_printer.line(virtual_path())+"\r\n");
  }
  
public:
  FTPListMlsd(IOController*fc,FileLister*fl,const FTPMlstPrinter&c):
    FTPList(fc,fl),
    _printer(c)
  {
  }

  std::string
  desc()const
  {
    return "MLSD " + FTPList::desc();
  }
};

#endif

#include <sstream>
#include <ctime>
#include <err.h>
#include <sys/stat.h>

#include <user.hh>
#include <path.hh>
#include "ftplistlong.hh"

static
std::string
ftp_date(std::time_t mtime)
{
  std::time_t now = std::time(0);
  struct tm tmbuf;
  struct tm *split_mtime = localtime_r(&mtime,&tmbuf);
  const char*date_format =  "%b %d %H:%M";
  char datebuf[64];
  
  if (mtime > now ||
      (now - mtime) > 60*60*12*365) // in the future or more than 6 months old
    date_format = "%b %d  %Y";

  int ret = strftime(datebuf,sizeof datebuf,date_format,split_mtime);
  datebuf[sizeof(datebuf)-1]=0;
  if(ret == 0) {
    warn("strftime");
    return  "Jan  1 00:00";
  }

  return datebuf;
}

void
FTPListLong::prepare_entry()
{
  User::Stat buf;
  std::ostringstream s;
  while(!_user.stat(virtual_path(),buf)){
    skip_entry();
    if(no_entries_left())
      return;
  }
    //   s << "---------- 0 unstatable deleted 0 Jan  1 00:00 ";
  s << buf.rwx_string() << ' ';
  s << buf.nlink() << ' ';
  s << buf.username() << ' ';
  s << buf.groupname() << ' ';
  s << buf.size() << ' ';
  s << ftp_date(buf.mtime()) << ' ';
  s << entry_name() << "\r\n";
  set_entry(s.str());
}

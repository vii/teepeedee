#include "serverftp.hh"
#include "ftpcontrol.hh"
#include "serverregistration.hh"

static ServerRegistration registration("ftp",ServerFTP::factory,"FTP file server");

IOContext*
ServerFTP::new_iocontext()
{
  return new FTPControl(config(),stream_container(),_ssl_available?this:0);
}

#ifdef HAVE_OPENSSL
#include "serverhttps.hh"
#include "serverregistration.hh"
#include "httpcontrol.hh"

static ServerRegistration registration("https",ServerHTTPS::factory,"Secure HTTPS web server");

IOContext*
ServerHTTPS::new_iocontext()
{
  return new HTTPControl(config(),"https");  
}

#endif

#include "serverhttp.hh"
#include "httpcontrol.hh"
#include "serverregistration.hh"

static ServerRegistration registration("http",ServerHTTP::factory,"HTTP web server");

IOContext*
ServerHTTP::new_iocontext()
{
  return new HTTPControl(config(),stream_container());
}

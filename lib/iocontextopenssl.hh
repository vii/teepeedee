#ifndef _TEEPEEDEE_LIB_IOCONTEXTOPENSSL_HH
#define _TEEPEEDEE_LIB_IOCONTEXTOPENSSL_HH

#ifndef _TEEPEEDEE_LIB_IOCONTEXTOPENSSL_I_KNOW_ABOUT_OPENSSL
#error Sorry, to you I do not exist
#endif

#include <err.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>

#include "stream.hh"
#include "iocontextwriter.hh"
#include "opensslexception.hh"

class IOContextOpenSSL : public IOContextWriter
{
  typedef IOContextWriter super;
  
  unsigned ssl_buffer_max()const
  {
    return 1<<16;
  }
  bool ssl_buffer_too_big(BIO*bio)
  {
    BUF_MEM *bptr;
    BIO_get_mem_ptr(bio, &bptr);
    return(bptr->length > (int)ssl_buffer_max());
  }
  bool ssl_write_buffer_too_big()
  {
    return ssl_buffer_too_big(_ssl_out);
  }
  
  bool
  waiting(int ret)
  {
    if(SSL_get_error(_ssl,ret)==SSL_ERROR_WANT_READ)
      return true;
    if(SSL_get_error(_ssl,ret)==SSL_ERROR_WANT_WRITE)
      return true;
    return false;
  }
  
  SSL*_ssl;
  BIO*_ssl_in; // data going into _SSL
  BIO*_ssl_out; // data coming out of _SSL

  struct State
  {
    enum type { none,established,accept,shutdown,hungup };
  };
  State::type _state;

  void
  try_accept()
  {
    int ret = SSL_accept(_ssl);
    if(ret == 1){
      _state = State::established;
    } else {
      if(ret >= 0 || !waiting(ret))
	throw OpenSSLException("SSL_accept");
    }
  }
    
  void
  try_shutdown()
  {
    int ret = SSL_shutdown(_ssl);
    if(ret == -1){
      if(!waiting(ret))
	throw OpenSSLException("SSL_shutdown");
    }
    if(SSL_get_shutdown(_ssl)&SSL_SENT_SHUTDOWN){
      _state = State::none;
      delete_this();
      return;
    }
  }
  void
  try_state_change()
  {
    switch(_state){
    case State::accept:
      try_accept();
      break;
    case State::shutdown:
      try_shutdown();
      break;
    default:break;
      
    }
  } 
  void
  do_set_write_buf()
  {
    int left = BIO_pending(_ssl_out);
    BUF_MEM *bptr;
    BIO_get_mem_ptr(_ssl_out, &bptr);
    move_write_buf(bptr->data,left);
  }
  
  friend class StreamOpenSSL;
protected:

  virtual
  void
  read_in(Stream&stream,size_t max)
  {
    if(dead()){
      if(!BIO_pending(_ssl_in))
	delete_this();
      return;
    }
    
    if(ssl_buffer_too_big(_ssl_in)){
      if(_state == State::shutdown || _state == State::accept) {
	warnx("SSL input buffer too full; rudely shutting down");
	delete_this();
      }
      return;
    }

    try_state_change();

    char buf[16000]; // must be separate buffer because ssl might require protocol reads to continue writes
    
    max = (max && max < sizeof buf) ? max : sizeof buf;
    ssize_t ret = do_read(stream,buf,max);
    if(ret == -1)
      return;
    if(ret == 0) {
      hangup(stream);
      return;
    }
    if(ret != BIO_write(_ssl_in, buf, ret))
      throw OpenSSLException("BIO_write");
  }
  void
  write_out(Stream&stream,size_t max)
  {
    if(dead()){
      return;
    }
    do_set_write_buf();

    super::write_out(stream,max);
  }
  void
  finished_writing()
  {
    if(dead()){
      return;
    }
    try_state_change();
    
    if(buf_len() != (unsigned)BIO_pending(_ssl_out)) {
      do_set_write_buf();
      return;
    }
    BIO_reset(_ssl_out);
    reset_write_buf();
  }

  SSL*
  ssl()
  {
    return _ssl;
  }

  void accept()
  {
    _state = State::accept;
    try_accept();
  }
  void
  shutdown()
  {
    if(!dead())
      _state = State::shutdown;
  }

  bool
  dead()const
  {
    return _state == State::hungup || _state==State::none;
  }
  bool
  hungup()const
  {
    return _state == State::hungup;
  }
  
public:
  IOContextOpenSSL(SSL_CTX*ctx):_ssl(0),
					 _ssl_in(0),
					 _ssl_out(0),
					 _state(State::none)
  {
    _ssl = SSL_new(ctx);
    if(!_ssl)
      throw OpenSSLException("SSL_new");
    _ssl_in = BIO_new(BIO_s_mem());
    if(!_ssl_in){
      SSL_free(_ssl);
      throw OpenSSLException("BIO_new (input BIO)");
    }
    _ssl_out = BIO_new(BIO_s_mem());
    if(!_ssl_out){
      BIO_free(_ssl_in);
      SSL_free(_ssl);
      throw OpenSSLException("BIO_new (output BIO)");
    }
    SSL_set_bio(_ssl,_ssl_in,_ssl_out);
  }

  std::string
  desc()const
  {
    return "ssl filter";
  }

  ~IOContextOpenSSL()
  {
    if(_ssl){
      SSL_free(_ssl);
      _ssl = 0;
    }
  }
  bool
  want_read(Stream&)
  {
    return !ssl_buffer_too_big(_ssl_in);
  }
  bool
  stream_hungup(Stream&stream)
  {
    if(_state == State::none)
      return true;
    if(_state == State::shutdown)
      return true;
    _state = State::hungup;
    return false;
  }
};


#endif

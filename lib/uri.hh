#ifndef _TEEPEEDEE_LIB_URI_HH
#define _TEEPEEDEE_LIB_URI_HH

#include <string>

#include "path.hh"

// partial RFC 2396
// does not handle relative paths

class URI
{
  std::string _scheme;
  std::string _authority;
  Path _path;

  void
  reset()
  {
    _scheme = std::string();
    _authority = std::string();
    _path.clear();
  }
  
  void
  parse_from_authority(const std::string&encoded)
    ;
  
  void
  parse_from_path(const std::string&encoded)
    ;

  static
  std::string
  dequote_percent(const std::string&hexencoded)
    ;

  static
  std::string
  quote_percent(const std::string&hexencoded)
    ;

  
public:
  URI(const std::string&e) // e should be encoded as in a http request
  {
    parse(e);
  }
  URI()
  {
  }
  
  Path
  path()const
  {
    return _path;
  }
  void
  set_path(const Path&unencoded)
  {
    _path = unencoded;
  }

  std::string
  str()const
  {
    return get_html_quoted();
  }

  std::string
  get_html_quoted()const
    ;

  void
  parse(const std::string&encoded)
    ;
};

#endif

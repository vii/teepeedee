#include <cctype>
#include "uri.hh"

// XXX does not work on strange machines without ascii

static
char
hexdigit(char c)
{
  if(c >= '0' && c <= '9')
    return c - '0';
  if(c >= 'A' && c <= 'F')
    return c + 10 - 'A';
  if(c >= 'a' && c <= 'f')
    return c + 10 - 'a';
  return 0;
}

static
char
hexdecode(char i1,char i2)
{
  return hexdigit(i1) << 4 | hexdigit(i2);
}
static
char
hexdigitise(int c)
{
  if(0 <= c && 9 >= c)
    return c + '0';
  if(10 <= c && 16 > c)
    return c + 'A' - 10;
  return '!';
}

static
std::string
hexencode(char c)
{
  std::string ret;
  ret += hexdigitise((c >> 4) & 0xf);
  ret += hexdigitise((c)&0xf);
  return ret;
}

std::string
URI::quote_percent(const std::string&unquoted)
{
  using std::isalnum; // cannot use std::isalnum directly as not portable to gcc-2.95 + glibc-2
  
  std::string ret;
  for(std::string::const_iterator i = unquoted.begin();
      i != unquoted.end(); ++i){
    if(isalnum(*i) 
       || *i == '-'
       || *i == '_'
       || *i == '.'
       || *i == '!'
       || *i == '~'
       || *i == '*'
       || *i == '\''
       || *i == '('
       || *i == ')'
       || *i == '/'){ // XXX should not have '/' FIXME
      ret += *i;
    } else {
      ret += '%';
      ret += hexencode(*i);
    }
  }
  return ret;  
}

std::string
URI::dequote_percent(const std::string&hexencoded)
{
  if(hexencoded.empty())
    return std::string();
  
  std::string ret;

  for(const char*i=hexencoded.c_str();*i;++i){
    if(*i == '%'){
      if(!i[1] || !i[2])
	break;
      ret += hexdecode(i[1],i[2]);
      i += 2;
    }
    else
      ret += *i;
  }

  return ret;
}

std::string
URI::get_html_quoted()const
{
  std::string ret;
  if(!_scheme.empty()){
    ret += quote_percent(_scheme);
    ret += ":/";
    if(!_authority.empty())
      ret += "/";
  }
  ret += quote_percent(_authority);
  if(!_authority.empty())
    ret += '/';
  ret += quote_percent(_path.str());
  
  return ret;  
}

void
URI::parse_from_authority(const std::string&str)
{
  std::string::size_type len = str.length();

  for(std::string::size_type i=0;i<len;++i)
    if(str[i] == '/') {
      _authority = dequote_percent(str.substr(0,i));
      return parse_from_path(str.substr(i));
    }
  _authority = dequote_percent(str);
}

void
URI::parse_from_path(const std::string&str)
{
  _path.append(dequote_percent(str));
}

void
URI::parse(const std::string&str)
{
  reset();
  
  if(str.empty())
    return;

  std::string::size_type start;
  std::string::size_type len = str.length();

  // get scheme
  for(start=0; start < len; ++start) {
    if(str[start] == '/') {
      return parse_from_path(str);
    }
    if(str[start] == ':'){
      _scheme = dequote_percent(str.substr(0,start));
      if(++start >= len)
	return;
      if(str[start] == '/'){
	if(++start >= len)
	  return;
	if(str[start] == '/'){
	  parse_from_authority(str.substr(start+1));
	  return;
	}
	parse_from_path(str.substr(start));
	return;
      }
    }
  }
  parse_from_path(str);
}

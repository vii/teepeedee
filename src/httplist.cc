#include <sstream>

#include <filelister.hh>
#include <uri.hh>

#include "httplist.hh"

HTTPList::HTTPList(IOController*fc,FileLister*fl,const User&u)  :
  IOContextList(fc,fl),
  _finished(false),
  _user(u)
{
  Path p(virtual_path());
  if(!p.empty())
    p.pop_back();
  std::string name = p.str();
  std::string head = "<head><title>Directory listing of " + name + "/</title></head>\n"
	    "<body>\n";

  std::string welcome = _user.message_login();
  if(!welcome.empty())
    head += "<pre>" + welcome + "</pre>\n";
  head += "<h1>Directory listing of " + name + "/</h1>\n";
  welcome = _user.message_directory(p);
  if(!welcome.empty())
    head += "<pre>" + welcome + "</pre>\n";
  head += "<ul>\n";
  set_entry(head);
}

void
HTTPList::prepare_entry()
{
  URI uri;
  uri.set_path(virtual_path());
  User::Stat buf;
  std::string name = entry_name();
  std::string extra_info;
  try {
    if(_user.stat(virtual_path(),buf)){
      if(buf.is_dir())
	name += "/";
      else {
	std::ostringstream s;
	s << " (" << buf.size() << " bytes)";
	extra_info = s.str();
      }
    }
  } catch (const User::AccessException&){
  }
  
  set_entry("<LI><A href=\"/" + uri.get_html_quoted() + "\">" + name + "</A>" + extra_info + "</LI>\n");
}

void
HTTPList::finished_listing()
{
  if(!_finished){
    set_entry("</ul></body>");
    _finished = true;
  } else {
    IOContextList::finished_listing();
  }
}

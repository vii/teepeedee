#ifndef _TEEPEEDEE_LIB_FILELISTERGLOB_HH
#define _TEEPEEDEE_LIB_FILELISTERGLOB_HH

#include <string>
#include "filelister.hh"
#include "path.hh"

class FileListerGlob:public FileLister
{
  std::string _pattern;
  FileLister* _lister;

  void
  free()
  {
    if(_lister){
      delete _lister;
      _lister = 0;
    }
  }
  static
  bool
  match(std::string::const_iterator nbeg,
	std::string::const_iterator nend,
	std::string::const_iterator pbeg,
	std::string::const_iterator pend)
  {
    while(pbeg != pend){
      if(*pbeg == '*'){
	do {
	  ++pbeg;
	  if(pbeg == pend)
	    return true;
	} while(*pbeg == '*'); // stop DoS attack
	
	for(std::string::const_iterator i = nbeg;i!=nend;++i)
	  if(match(i,nend,pbeg,pend))
	    return true;
	return false;
      }
      if(nbeg == nend)
	return false;
      if(*pbeg != '?')
	if(*nbeg != *pbeg)
	  return false;
      ++nbeg;
      ++pbeg;
    }
    
    return nbeg == nend;
  }
  bool
  match(const std::string&name)
  {
    return match(name.begin(),name.end(),_pattern.begin(),_pattern.end());
  }
  void
  skip_till_match()
  {
    if(!_lister)return;
    while (!_lister->last() && !match(_lister->name())){
      _lister->next();
    }
  }
  
public:
  FileListerGlob(const std::string&pat,FileLister*list):
    _pattern(pat),
    _lister(list)
  {
    skip_till_match();
  }
  ~FileListerGlob()
  {
    free();
  }
  
  virtual
  bool
  last()const {
    return !_lister || _lister->last();
  }
  
  virtual void
  next() {
    if(last())return;
    _lister->next();
    skip_till_match();
  }
  
  virtual
  std::string
  name()
    const
  {
    return _lister->name();
  }
  virtual
  Path
  virtual_path()
    const
  {
    return _lister->virtual_path();
  }
};

#endif

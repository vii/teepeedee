#ifndef _TEEPEEDEE_CONFEXCEPTION_HH
#define _TEEPEEDEE_CONFEXCEPTION_HH

#include <exception>

class ConfException : public std::exception
{
  std::string _value_name;
  std::string _problem;
public:
  ConfException(const std::string&val,const std::string&prob):
    _value_name(val)
  {
    _problem = _value_name + ": " + prob;
  }

  const char*what() const throw() 
  {
    return _problem.c_str();
  }

  ~ConfException()throw()
  {
  }
  
};

#endif

#ifndef _TEEPEEDEE_LIB_LIMITEXCEPTION_HH
#define _TEEPEEDEE_LIB_LIMITEXCEPTION_HH

#include <exception>

class LimitException:public std::exception{
public:
  const char*
  what()const throw()
  {
    return "configured limit exceeded";
  }
  ~LimitException()throw()
  {
  }
  
};


#endif

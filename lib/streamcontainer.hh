#ifndef _TEEPEEDEE_LIB_STREAMCONTAINER_HH
#define _TEEPEEDEE_LIB_STREAMCONTAINER_HH

class Stream;

// This is just to hide StreamTable from things
class StreamContainer 
{
public:
  virtual
  void
  add(Stream*s)
    =0;
  virtual
  ~StreamContainer()
  {
  }
};

#endif

#include <msvcrti.h>


int _ismbbalnum(unsigned int c)
{
  return (isalnum(c) || _ismbbkalnum(c));
}


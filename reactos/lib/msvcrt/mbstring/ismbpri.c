#include <msvcrti.h>


int _ismbbprint(unsigned int c)
{
  return (isprint(c) || _ismbbkana(c));
}

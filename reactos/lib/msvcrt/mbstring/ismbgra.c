#include <msvcrti.h>


int _ismbbgraph(unsigned int c)
{
  return (isgraph(c) || _ismbbkana(c));
}

#include <msvcrt/mbstring.h>
#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

int _ismbbgraph(unsigned char c)
{
  return (isgraph(c) || _ismbbkana(c));
}

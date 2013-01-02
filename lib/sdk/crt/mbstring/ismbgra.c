#include <mbstring.h>
#include <mbctype.h>
#include <ctype.h>

/*
 * @implemented
 */
int _ismbbgraph(unsigned int c)
{
  return (isgraph(c) || _ismbbkana(c));
}

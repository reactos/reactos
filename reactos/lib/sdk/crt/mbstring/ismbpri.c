#include <mbstring.h>
#include <mbctype.h>
#include <ctype.h>

/*
 * @implemented
 */
int _ismbbprint(unsigned int c)
{
  return (isprint(c) || _ismbbkana(c));
}

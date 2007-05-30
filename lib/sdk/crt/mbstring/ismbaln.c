#include <mbctype.h>
#include <ctype.h>


/*
 * @implemented
 */
int _ismbbalnum(unsigned int c)
{
  return (isalnum(c) || _ismbbkalnum(c));
}


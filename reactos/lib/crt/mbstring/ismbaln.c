#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>


/*
 * @implemented
 */
int _ismbbalnum(unsigned char c)
{
  return (isalnum(c) || _ismbbkalnum(c));
}


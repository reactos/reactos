#include <msvcrt/string.h>

/*
 * @implemented
 */
unsigned char * _mbschr(const unsigned char *str, unsigned int c)
{
  return strchr(str,c);
}

#include <msvcrt/string.h>

unsigned char * _mbschr(const unsigned char *str, unsigned int c)
{
  return strchr(str,c);
}

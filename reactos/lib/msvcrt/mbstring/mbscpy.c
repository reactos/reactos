#include <msvcrt/mbstring.h>
#include <msvcrt/string.h>

unsigned char * _mbscpy(unsigned char *dst, const unsigned char *str)
{
  return strcpy(dst,str);
}

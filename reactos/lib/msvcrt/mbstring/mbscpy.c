#include <msvcrti.h>


unsigned char * _mbscpy(unsigned char *dst, const unsigned char *str)
{
  return strcpy(dst,str);
}

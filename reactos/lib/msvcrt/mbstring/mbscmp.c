#include <msvcrti.h>


int _mbscmp(const unsigned char *str1, const unsigned char *str2)
{
  return strcmp(str1,str2);
}

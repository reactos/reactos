#include <msvcrt/mbstring.h>
#include <msvcrt/string.h>

/*
 * @implemented
 */
int _mbscmp(const unsigned char *str1, const unsigned char *str2)
{
  return strcmp(str1,str2);
}

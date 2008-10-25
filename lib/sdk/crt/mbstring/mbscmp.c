#include <mbstring.h>
#include <string.h>

/*
 * @implemented
 */
int _mbscmp(const unsigned char *str1, const unsigned char *str2)
{
  return strcmp((const char*)str1, (char*)str2);
}

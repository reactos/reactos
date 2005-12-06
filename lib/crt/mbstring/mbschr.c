#include <string.h>

/*
 * @implemented
 */
unsigned char * _mbschr(const unsigned char *str, unsigned int c)
{
  return (unsigned char *)strchr((const char*)str, c);
}

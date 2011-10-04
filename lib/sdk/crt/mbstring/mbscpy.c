#include <precomp.h>
#include <mbstring.h>
#include <string.h>

/*
 * @implemented
 */
unsigned char * _mbscpy(unsigned char *dst, const unsigned char *str)
{
  return (unsigned char*)strcpy((char*)dst,(const char*)str);
}

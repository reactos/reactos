#include <precomp.h>
#include <string.h>

/*
 * @implemented
 */
unsigned char * _mbscat(unsigned char *dst, const unsigned char *src)
{
  return (unsigned char *)strcat((char*)dst,(const char*)src);
}

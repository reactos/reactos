#include <msvcrt/mbstring.h>
#include <msvcrt/string.h>

size_t _mbclen2(const unsigned int s);

/*
 * @implemented
 */
void _mbccpy(unsigned char *dst, const unsigned char *src)
{
  if (!_ismbblead(*src) )
    return;

  memcpy(dst,src,_mbclen2(*src));
}

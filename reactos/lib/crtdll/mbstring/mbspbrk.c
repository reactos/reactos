#include <msvcrt/mbstring.h>
/*
 * FIXME not correct
 *
 * @implemented
 */
unsigned char * _mbspbrk(const unsigned char *s1, const unsigned char *s2)
{
  const char *scanp;
  int c, sc;

  while ((c = *s1++) != 0)
  {
    for (scanp = s2; (sc = *scanp++) != 0;)
      if (sc == c)
	return (unsigned char *)((char *)s1 - (char *)1);
  }
  return 0;
}

#include <crtdll/mbstring.h>
// not correct
size_t _mbscspn(const unsigned char *s1, const unsigned char *s2)
{
  const char *p, *spanp;
  char c, sc;

  for (p = s1;;)
  {
    c = *p++;
    spanp = s2;
    do {
      if ((sc = *spanp++) == c)
	return (size_t)(p - 1) - (size_t)s1;
    } while (sc != 0);
  }
  /* NOTREACHED */
}

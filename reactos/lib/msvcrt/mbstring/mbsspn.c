#include <msvcrt/mbstring.h>

// not correct
size_t _mbsspn(const unsigned char *s1, const unsigned char *s2)
{
  const char *p = s1, *spanp;
  char c, sc;

 cont:
  c = *p++;
  for (spanp = s2; (sc = *spanp++) != 0;)
    if (sc == c)
      goto cont;
  return (size_t)(p - 1) - (size_t)s1;
// - (char *)s1);
}
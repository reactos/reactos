#include <crtdll/mbstring.h>

int _mbsnicmp(const unsigned char *s1, const unsigned char *s2, size_t n)
{
  if (n == 0)
    return 0;
  do {
    if (_mbbtoupper(*s1) != _mbbtoupper(*s2))
      return _mbbtoupper(*(unsigned const char *)s1) - _mbbtoupper(*(unsigned const char *)s2);
    *s1 += _mbclen2(*s1);
    *s2 += _mbclen2(*s2);


    if (*s1 == 0)
      break;
    if (!_ismbblead(*s1) )
	n--;
  } while (n > 0);
  return 0;
}
int _mbsnbicmp(const unsigned char *s1, const unsigned char *s2, size_t n)
{
  if (n == 0)
    return 0;
  do {
    if (_mbbtoupper(*s1) != _mbbtoupper(*s2))
      return _mbbtoupper(*(unsigned const char *)s1) - _mbbtoupper(*(unsigned const char *)s2);
    s1 += _mbclen2(*s1);
    s2 += _mbclen2(*s2);


    if (*s1 == 0)
      break;
    n--;
  } while (n > 0);
  return 0;
}
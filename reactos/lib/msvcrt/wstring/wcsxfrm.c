#include <msvcrt/string.h>

size_t wcsxfrm(wchar_t *dst,const wchar_t *src, size_t n)
{
  size_t r = 0;
  int c;

  if (n != 0) {
    while ((c = *src++) != 0)
    {
      r++;
      if (--n == 0)
      {
	while (*src++ != 0)
	  r++;
	break;
      }
      *dst++ = c;
    }
    *dst = 0;
  }
  return r;
}

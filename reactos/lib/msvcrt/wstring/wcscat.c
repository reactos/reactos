

#include <msvcrt/string.h>

wchar_t * wcscat(wchar_t * dest,const wchar_t * src)
{
  wchar_t *d = dest;
  for (; *dest !=0; dest++);
  while (*src != 0)
  {
	*dest = *src;
	dest++;
	src++;
  }
  *dest = 0;
  return d;
}


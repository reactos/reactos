

#include <crtdll/wchar.h>

wchar_t * wcscat(wchar_t * dest,const wchar_t * src)
{
  
  int i,j;
  j=0;
  for (i=0; dest[i]!=0; i++);
  while (src[j] != 0)
  {
	dest[i+j] = src[j];
	j++;
  }
  dest[i+j] = 0;
  return dest;
}


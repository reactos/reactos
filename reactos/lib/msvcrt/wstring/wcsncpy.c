#include <msvcrt/wchar.h>

wchar_t * wcsncpy(wchar_t * dest,const wchar_t *src,size_t count)
{
   int i;
   
   for (i=0;i<count;i++)
     {
	dest[i] = src[i];
	if (src[i] == 0)
	  {
	     return(dest);
	  }
     }
   return(dest);
}





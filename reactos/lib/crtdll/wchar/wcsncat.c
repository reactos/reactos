#include <crtdll/wchar.h>

wchar_t * wcsncat(wchar_t * dest,const wchar_t * src,size_t count)
{
   int i,j;
   if ( count != 0 ) {
   
   	for (j=0;dest[j]!=0;j++);

   	for (i=0;i<count;i++)
     	{
		dest[j+i] = src[i];
		if (src[i] == 0)
	     		return(dest);
	
     	}
   	dest[j+i]=0;
   }
   return(dest);
}

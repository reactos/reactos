#include <crtdll/string.h>


void * memmove(void *dest,const void *src,size_t count)
{
	char *char_dest = (char *)dest;
	char *char_src = (char *)src;

	while(count > 0 )
	{
		*char_dest = *char_src;
		char_dest++;
		char_src++;
		count--;
	}

	return dest;
	
	
}

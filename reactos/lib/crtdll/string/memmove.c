#include <string.h>


void * memmove(void *dest,const void *src,size_t count)
{
	int *int_dest = dest;
	int *int_src = src;

	while(count > 0 )
	{
		*int_dest = *int_src;
		int_dest++;
		int_src++;
		count--;
	}

	return dest;
	
	
}
#include <msvcrti.h>


void *
_memccpy (void *to, const void *from,int c,size_t count)
{
	memcpy(to,from,count);
	return memchr(to,c,count);
}

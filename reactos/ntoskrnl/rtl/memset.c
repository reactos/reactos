#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


void * memset(void *src, int val, size_t count)
{
	char *char_src = (char *)src;

	while(count>0) {
		*char_src = val;
		char_src++;
		count--;
	}
	return src;
}

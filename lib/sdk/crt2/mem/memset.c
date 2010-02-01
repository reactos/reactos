/*
 * $Id: memset.c 30266 2007-11-08 10:54:42Z fireball $
 */

// #include <string.h>

_NOINTRINSIC(memset)

void * _CDECL memset(void* src, int val, size_t count)
{
	char *char_src = (char *)src;

	while(count) {
		*char_src = val;
		char_src++;
		count--;
	}
	return src;
}

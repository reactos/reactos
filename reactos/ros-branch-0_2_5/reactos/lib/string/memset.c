/*
 * $Id: memset.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

void* memset(void* src, int val, size_t count)
{
	char *char_src = (char *)src;

	while(count>0) {
		*char_src = val;
		char_src++;
		count--;
	}
	return src;
}

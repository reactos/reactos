
#include <windows.h>
#include <port1632.h>
#include "recordll.h"


LPVOID lmemcpy(LPVOID dst, LPVOID src, INT count)
{
LPSTR bsrc = (LPSTR)src;
LPSTR bdst = (LPSTR)dst;

	if (bdst <= bsrc || bdst >= (bsrc + count)) {
		/*
		 * Non-Overlapping Buffers
		 * copy from lower addresses to higher addresses
		 */
		while (count--)
			*bdst++ = *bsrc++;
		}
	else {
		/*
		 * Overlapping Buffers
		 * copy from higher addresses to lower addresses
		 */
		bdst += count - 1;
		bsrc += count - 1;

		while (count--)
			*bdst-- = *bsrc--;
		}

	return(dst);
}

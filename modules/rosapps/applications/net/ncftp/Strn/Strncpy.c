#include <sys/types.h>
#include <string.h>
#include "Strn.h"


/*
 * Copy src to dst, truncating or null-padding to always copy n-1 bytes.
 * Return dst.
 */
char *
Strncpy(char *const dst, const char *const src, const size_t n)
{
	register char *d;
	register const char *s;
	register size_t i;

	d = dst;
	*d = 0;
	if (n != 0) {
		s = src;
		/* If they specified a maximum of n characters, use n - 1 chars to
		 * hold the copy, and the last character in the array as a NUL.
		 * This is the difference between the regular strncpy routine.
		 * strncpy doesn't guarantee that your new string will have a
		 * NUL terminator, but this routine does.
		 */
		for (i=1; i<n; i++) {
			if ((*d++ = *s++) == 0) {
#if (STRN_ZERO_PAD == 1)
				/* Pad with zeros. */
				for (; i<n; i++)
					*d++ = 0;
#endif	/* STRN_ZERO_PAD */
				return dst;
			}
		}
		/* If we get here, then we have a full string, with n - 1 characters,
		 * so now we NUL terminate it and go home.
		 */
		*d = 0;
	}
	return (dst);
}	/* Strncpy */

/* eof Strn.c */

#include <sys/types.h>
#include <string.h>
#include "Strn.h"

/*
 * Copy src to dst, truncating or null-padding to always copy n-1 bytes.
 *
 * This routine differs from strncpy in that it returns a pointer to the end
 * of the buffer, instead of strncat which returns a pointer to the start.
 */
char *
Strnpcpy(char *const dst, const char *const src, size_t n)
{
	register char *d;
	register const char *s;
	register char c;
	char *ret;
	register size_t i;

	d = dst;
	if (n != 0) {
		s = src;
		/* If they specified a maximum of n characters, use n - 1 chars to
		 * hold the copy, and the last character in the array as a NUL.
		 * This is the difference between the regular strncpy routine.
		 * strncpy doesn't guarantee that your new string will have a
		 * NUL terminator, but this routine does.
		 */
		for (i=1; i<n; i++) {
			c = *s++;
			if (c == '\0') {
				ret = d;	/* Return ptr to end byte. */
				*d++ = c;
#if (STRNP_ZERO_PAD == 1)
				/* Pad with zeros. */
				for (; i<n; i++)
					*d++ = 0;
#endif	/* STRNP_ZERO_PAD */
				return ret;
			}
			*d++ = c;
		}
		/* If we get here, then we have a full string, with n - 1 characters,
		 * so now we NUL terminate it and go home.
		 */
		*d = '\0';
		return (d);	/* Return ptr to end byte. */
	} else {
		*d = 0;
	}
	return (d);	/* Return ptr to end byte. */
}	/* Strnpcpy */

/* eof Strn.c */

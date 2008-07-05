#include <sys/types.h>
#include <string.h>
#include "Strn.h"

/*
 * Concatenate src on the end of dst.  The resulting string will have at most
 * n-1 characters, not counting the NUL terminator which is always appended
 * unlike strncat.  The other big difference is that strncpy uses n as the
 * max number of characters _appended_, while this routine uses n to limit
 * the overall length of dst.
 *
 * This routine also differs in that it returns a pointer to the end
 * of the buffer, instead of strncat which returns a pointer to the start.
 */
char *
Strnpcat(char *const dst, const char *const src, size_t n)
{
	register size_t i;
	register char *d;
	register const char *s;
	register char c;
	char *ret;

	if (n != 0 && ((i = strlen(dst)) < (n - 1))) {
		d = dst + i;
		s = src;
		/* If they specified a maximum of n characters, use n - 1 chars to
		 * hold the copy, and the last character in the array as a NUL.
		 * This is the difference between the regular strncpy routine.
		 * strncpy doesn't guarantee that your new string will have a
		 * NUL terminator, but this routine does.
		 */
		for (++i; i<n; i++) {
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
		*d = 0;
		return (d);	/* Return ptr to end byte. */
	}
	return (dst);
}	/* Strnpcat */

/* strtokc.c */

#include <string.h>
#include "Strn.h"

char *
strtokc(char *parsestr, const char *delims, char **context)
{
	char *cp;
	const char *cp2;
	char c, c2;
	char *start;

	if (parsestr == NULL)
		start = *context;
	else
		start = parsestr;

	if ((start == NULL) || (delims == NULL)) {
		*context = NULL;
		return NULL;
	}

	/* Eat leading delimiters. */
	for (cp = start; ; ) {
next1:
		c = *cp++;
		if (c == '\0') {
			/* No more tokens. */
			*context = NULL;
			return (NULL);
		}
		for (cp2 = delims; ; ) {
			c2 = (char) *cp2++;
			if (c2 == '\0') {
				/* This character was not a delimiter.
				 * The token starts here.
				 */
				start = cp - 1;
				goto starttok;
			}
			if (c2 == c) {
				/* This char was a delimiter. */
				/* Skip it, look at next character. */
				goto next1;
			}
		}
		/*NOTREACHED*/
	}

starttok:
	for ( ; ; cp++) {
		c = *cp;
		if (c == '\0') {
			/* Token is finished. */
			*context = cp;
			break;
		}
		for (cp2 = delims; ; ) {
			c2 = (char) *cp2++;
			if (c2 == '\0') {
				/* This character was not a delimiter.
				 * Keep it as part of current token.
				 */
				break;
			}
			if (c2 == c) {
				/* This char was a delimiter. */
				/* End of token. */
				*cp++ = '\0';
				*context = cp;
				return (start);
			}
		}
	}
	return (start);
}	/* strtokc */




/* Same as strtokc, only you specify the destination buffer to write
 * the token in along with its size.  strntokc will write to the dst
 * buffer, always nul-terminating it.
 *
 * It also returns the length of the token, or zero if there was no
 * token.  This differs from strtokc, which returns a pointer to the
 * token or NULL for no token.
 */

int
strntokc(char *dstTokenStart, size_t tokenSize, char *parsestr, const char *delims, char **context)
{
	char *cp;
	const char *cp2;
	char c, c2;
	char *start;
	int len;
	char *dst, *lim;

	dst = dstTokenStart;
	lim = dst + tokenSize - 1;		/* Leave room for nul byte. */

	if (parsestr == NULL)
		start = *context;
	else
		start = parsestr;

	if ((start == NULL) || (delims == NULL)) {
		*context = NULL;
		goto done;
	}

	/* Eat leading delimiters. */
	for (cp = start; ; ) {
next1:
		c = *cp++;
		if (c == '\0') {
			/* No more tokens. */
			*context = NULL;
			goto done;
		}
		for (cp2 = delims; ; ) {
			c2 = (char) *cp2++;
			if (c2 == '\0') {
				/* This character was not a delimiter.
				 * The token starts here.
				 */
				start = cp - 1;
				if (dst < lim)
					*dst++ = c;
				goto starttok;
			}
			if (c2 == c) {
				/* This char was a delimiter. */
				/* Skip it, look at next character. */
				goto next1;
			}
		}
		/*NOTREACHED*/
	}

starttok:
	for ( ; ; cp++) {
		c = *cp;
		if (c == '\0') {
			/* Token is finished. */
			*context = cp;
			break;
		}
		for (cp2 = delims; ; ) {
			c2 = (char) *cp2++;
			if (c2 == '\0') {
				/* This character was not a delimiter.
				 * Keep it as part of current token.
				 */
				break;
			}
			if (c2 == c) {
				/* This char was a delimiter. */
				/* End of token. */
				*cp++ = '\0';
				*context = cp;
				goto done;
			}
		}
		if (dst < lim)			/* Don't overrun token size. */
			*dst++ = c;
	}

done:
	*dst = '\0';
	len = (int) (dst - dstTokenStart);	/* Return length of token. */

#if (STRN_ZERO_PAD == 1)
	/* Pad with zeros. */
	for (++dst; dst <= lim; )
		*dst++ = 0;
#endif	/* STRN_ZERO_PAD */

	return (len);
}	/* strntokc */




#ifdef TESTING_STRTOK
#include <stdio.h>

void
main(int argc, char **argv)
{
	char buf[256];
	int i;
	char *t;
	char token[8];
	int tokenLen;
	char *context;

	if (argc < 3) {
		fprintf(stderr, "Usage: test \"buffer,with,delims\" <delimiters>\n");
		exit(1);
	}
	strcpy(buf, argv[1]);
	i = 1;
	t = strtok(buf, argv[2]);
	if (t == NULL)
		exit(0);
	do {
		printf("strtok %d=[%s] length=%d\n", i, t, (int) strlen(t));
		t = strtok(NULL, argv[2]);
		++i;
	} while (t != NULL);

	printf("------------------------------------------------\n");
	strcpy(buf, argv[1]);
	i = 1;
	t = strtokc(buf, argv[2], &context);
	if (t == NULL)
		exit(0);
	do {
		printf("strtokc %d=[%s] length=%d\n", i, t, (int) strlen(t));
		t = strtokc(NULL, argv[2], &context);
		++i;
	} while (t != NULL);

	printf("------------------------------------------------\n");
	strcpy(buf, argv[1]);
	i = 1;
	tokenLen = strntokc(token, sizeof(token), buf, argv[2], &context);
	if (tokenLen <= 0)
		exit(0);
	do {
		printf("strntokc %d=[%s] length=%d\n", i, token, tokenLen);
		tokenLen = strntokc(token, sizeof(token), NULL, argv[2], &context);
		++i;
	} while (tokenLen > 0);
	exit(0);
}
#endif

/* Strntok.c */

#include <string.h>
#include "Strn.h"

/* This version of Strtok differs from the regular ANSI strtok in that
 * an empty token can be returned, and consecutive delimiters are not
 * ignored like ANSI does.  Example:
 *
 * Parse String = ",,mike,gleason,-West Interactive,402-573-1000"
 * Delimiters = ",-"
 *
 * (ANSI strtok:)
 * strtok 1=[mike] length=4
 * strtok 2=[gleason] length=7
 * strtok 3=[West Interactive] length=16
 * strtok 4=[402] length=3
 * strtok 5=[573] length=3
 * strtok 6=[1000] length=4
 * 
 * (Strtok:)
 * Strtok 1=[] length=0
 * Strtok 2=[] length=0
 * Strtok 3=[mike] length=4
 * Strtok 4=[gleason] length=7
 * Strtok 5=[] length=0
 * Strtok 6=[West Interactive] length=16
 * Strtok 7=[402] length=3
 * Strtok 8=[573] length=3
 * Strtok 9=[1000] length=4
 *
 */

char *
Strtok(char *buf, const char *delims)
{
	static char *p = NULL;
	char *start, *end;

	if (buf != NULL) {
		p = buf;
	} else {
		if (p == NULL)
			return (NULL);		/* No more tokens. */
	}
	for (start = p, end = p; ; end++) {
		if (*end == '\0') {
			p = NULL;		/* This is the last token. */
			break;
		}
		if (strchr(delims, (int) *end) != NULL) {
			*end++ = '\0';
			p = end; 
			break;
		}
	}
	return (start);
}	/* Strtok */




/* This is a bounds-safe version of Strtok, where you also pass a pointer
 * to the token to write into, and its size.  Using the example above,
 * with a char token[8], you get the following.  Notice that the token
 * is not overrun, and is always nul-terminated:
 *
 * Strntok 1=[] length=0
 * Strntok 2=[] length=0
 * Strntok 3=[mike] length=4
 * Strntok 4=[gleason] length=7
 * Strntok 5=[] length=0
 * Strntok 6=[West In] length=7
 * Strntok 7=[402] length=3
 * Strntok 8=[573] length=3
 * Strntok 9=[1000] length=4
 */

int
Strntok(char *dstTokenStart, size_t tokenSize, char *buf, const char *delims)
{
	static char *p = NULL;
	char *end;
	char *lim;
	char *dst;
	int len;

	dst = dstTokenStart;
	lim = dst + tokenSize - 1;		/* Leave room for nul byte. */

	if (buf != NULL) {
		p = buf;
	} else {
		if (p == NULL) {
			*dst = '\0';
			return (-1);		/* No more tokens. */
		}
	}

	for (end = p; ; end++) {
		if (*end == '\0') {
			p = NULL;		/* This is the last token. */
			break;
		}
		if (strchr(delims, (int) *end) != NULL) {
			++end;
			p = end; 
			break;
		}
		if (dst < lim)			/* Don't overrun token size. */
			*dst++ = *end;
	}
	*dst = '\0';
	len = (int) (dst - dstTokenStart);	/* Return length of token. */

#if (STRN_ZERO_PAD == 1)
	/* Pad with zeros. */
	for (++dst; dst <= lim; )
		*dst++ = 0;
#endif	/* STRN_ZERO_PAD */

	return (len);
}	/* Strntok */



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
	t = Strtok(buf, argv[2]);
	if (t == NULL)
		exit(0);
	do {
		printf("Strtok %d=[%s] length=%d\n", i, t, (int) strlen(t));
		t = Strtok(NULL, argv[2]);
		++i;
	} while (t != NULL);

	printf("------------------------------------------------\n");
	strcpy(buf, argv[1]);
	i = 1;
	tokenLen = Strntok(token, sizeof(token), buf, argv[2]);
	if (tokenLen < 0)
		exit(0);
	do {
		printf("Strntok %d=[%s] length=%d\n", i, token, tokenLen);
		tokenLen = Strntok(token, sizeof(token), NULL, argv[2]);
		++i;
	} while (tokenLen >= 0);
	exit(0);
}
#endif

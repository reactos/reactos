#include "fitz.h"
#include "samus.h"

/*
 * Create a new canonical absolute path given both
 * a base url (current directory) and a relative or absolute name.
 */
char *
sa_resolvepath(char *dst, char *baseurl, char *parturl, int dstlen)
{
	char *end;

	if (parturl[0] == '/')
	{
		strlcpy(dst, parturl, dstlen);
	}
	else
	{
		strlcpy(dst, baseurl, dstlen);
		end = strrchr(dst, '/');
		if (end)
			end[1] = 0;
		strlcat(dst, parturl, dstlen);
		cleanname(dst);
	}

	return dst;
}

/*
 * Test part names for equivalence.
 *
 * What we *should* do here (according to the spec) is...
 * - Convert part name to a Unicode string by un-escaping UTF-8 octets.
 * - Convert this to upper case.
 * - Normalize to NFC.
 *
 * But all we do is a case insensitive ASCII string comparison.
 */

static inline int toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		return c + 'A' - 'a';
	return c;
}

int sa_strcmp(char *a, char *b)
{
	while (toupper(*a) == toupper(*b++))
		if (*a++ == 0)
			return 0;
	return toupper(*a) - toupper(*(b-1));
}


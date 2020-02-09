
#include <precomp.h>

/*
 * @implemented
 */
char * CDECL _strlwr(char *x)
{
	char  *y=x;
	char ch, lower;

	while (*y) {
		ch = *y;
		lower = tolower(ch);
		if (ch != lower)
			*y = lower;
		y++;
	}
	return x;
}


#include <precomp.h>

/*
 * @implemented
 */
char * CDECL _strlwr(char *x)
{
	char  *y=x;

	while (*y) {
		*y=tolower(*y);
		y++;
	}
	return x;
}

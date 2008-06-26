
#include <precomp.h>

/*
 * @implemented
 */
char * _strlwr(char *x)
{
	char  *y=x;

	while (*y) {
		*y=tolower(*y);
		y++;
	}
	return x;
}

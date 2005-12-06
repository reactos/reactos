#include <string.h>
#include <ctype.h>


/*
 * @implemented
 */
char *_strupr(char *x)
{
	char  *y=x;

	while (*y) {
		*y=toupper(*y);
		y++;
	}
	return x;
}

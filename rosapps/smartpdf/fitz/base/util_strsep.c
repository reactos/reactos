#include <string.h>

char *strsep(char **stringp, const char *delim)
{
	char *ret = *stringp;
	if (ret == NULL) return NULL;
	if ((*stringp = strpbrk(*stringp, delim)) != NULL)
		*((*stringp)++) = '\0';
	return ret;
}


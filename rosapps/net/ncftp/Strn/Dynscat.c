#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "Strn.h"

/*VARARGS*/
char *
Dynscat(char **dst, ...)
{
	va_list ap;
	const char *src;
	char *newdst, *dcp;
	size_t curLen, catLen, srcLen;

	if (dst == (char **) 0)
		return NULL;

	catLen = 0;
	va_start(ap, dst);
	src = va_arg(ap, char *);
	while (src != NULL) {
		catLen += strlen(src);
		src = va_arg(ap, char *);
	}
	va_end(ap);

	if ((*dst == NULL) || (**dst == '\0'))
		curLen = 0;
	else
		curLen = strlen(*dst);

	if (*dst == NULL)
		newdst = malloc(curLen + catLen + 2);
	else
		newdst = realloc(*dst, curLen + catLen + 2);
	if (newdst == NULL)
		return NULL;

	dcp = newdst + curLen;
	va_start(ap, dst);
	src = va_arg(ap, char *);
	while (src != NULL) {
		srcLen = strlen(src);
		memcpy(dcp, src, srcLen);
		dcp += srcLen;
		src = va_arg(ap, char *);
	}
	va_end(ap);
	*dcp = '\0';

	*dst = newdst;
	return (newdst);
}	/* Dynscat */

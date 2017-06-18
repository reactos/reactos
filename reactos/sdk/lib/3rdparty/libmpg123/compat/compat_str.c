/*
	compat: Some compatibility functions (basic memory and string stuff)

	The mpg123 code is determined to keep it's legacy. A legacy of old, old UNIX.
	So anything possibly somewhat advanced should be considered to be put here, with proper #ifdef;-)

	copyright 2007-2016 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, Windows Unicode stuff by JonY.
*/

#include "compat.h"
#include "debug.h"

/* A safe realloc also for very old systems where realloc(NULL, size) returns NULL. */
void *safe_realloc(void *ptr, size_t size)
{
	if(ptr == NULL) return malloc(size);
	else return realloc(ptr, size);
}

#ifndef HAVE_STRERROR
const char *strerror(int errnum)
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	return (errnum < sys_nerr) ?  sys_errlist[errnum]  :  "";
}
#endif

char* compat_strdup(const char *src)
{
	char *dest = NULL;
	if(src)
	{
		size_t len;
		len = strlen(src)+1;
		if((dest = malloc(len)))
			memcpy(dest, src, len);
	}
	return dest;
}

/*
	compat: Some compatibility functions.

	The mpg123 code is determined to keep it's legacy. A legacy of old, old UNIX.
	So anything possibly somewhat advanced should be considered to be put here, with proper #ifdef;-)

	copyright 2007-8 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, Windows Unicode stuff by JonY.
*/

#include "config.h"
#include "compat.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <fcntl.h>
#endif

#ifdef WANT_WIN32_UNICODE
#include <wchar.h>
#include <windows.h>
#include <winnls.h>
#endif

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

#ifndef HAVE_STRDUP
char *strdup(const char *src)
{
	char *dest;

	if (!(dest = (char *) malloc(strlen(src)+1)))
	return NULL;
	else
	return strcpy(dest, src);
}
#endif

int compat_open(const char *filename, int mode)
{
	int ret;
#if defined (WANT_WIN32_UNICODE)
	const wchar_t *frag = NULL;

	ret = win32_utf8_wide(filename, &frag, NULL);
	if ((frag == NULL) || (ret == 0)) goto fallback; /* Fallback to plain open when ucs-2 conversion fails */

	ret = _wopen(frag, mode); /*Try _wopen */
	if (ret != -1 ) goto open_ok; /* msdn says -1 means failure */

fallback:
#endif

#ifdef __MSVCRT__ /* MSDN says POSIX function is deprecated beginning in Visual C++ 2005 */
	ret = _open (filename, mode); /* Try plain old _open(), if it fails, do nothing */
#else
	ret = open (filename, mode);
#endif

#if defined (WANT_WIN32_UNICODE)
open_ok:
	free ((void *)frag); /* Freeing a NULL should be OK */
#endif

	return ret;
}

int compat_close(int infd)
{
#ifdef __MSVCRT__ /* MSDN says POSIX function is deprecated beginning in Visual C++ 2005 */
	return _close(infd);
#else
	return close(infd);
#endif
}

/* Windows Unicode stuff */

#ifdef WANT_WIN32_UNICODE
int win32_wide_utf8(const wchar_t * const wptr, const char **const mbptr, size_t * const buflen)
{
	size_t len;
	char *buf;
	int ret;

	len = WideCharToMultiByte(CP_UTF8, 0, wptr, -1, NULL, 0, NULL, NULL); /* Get utf-8 string length */
	buf = calloc(len, sizeof (char)); /* Can we assume sizeof char always = 1? */
	debug2("win32_wide_utf8 allocated %u bytes at %p", len, buf);

	if(buf != NULL)
	{
		ret = WideCharToMultiByte(CP_UTF8, 0, wptr, -1, buf, len, NULL, NULL); /*Do actual conversion*/
		*mbptr = buf; /* Set string pointer to allocated buffer */
		if(buflen != NULL) *buflen = len * sizeof (char); /* Give length of allocated memory if needed. */

		return ret;
	}
	else
	{
		if(buflen != NULL) *buflen = 0;

		return 0;
	}
}

int win32_utf8_wide(const char *const mbptr, const wchar_t ** const wptr, size_t * const buflen)
{
	size_t len;
	wchar_t *buf;
	int ret;

	len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mbptr, -1, NULL, 0); /* Get converted size */
	buf = calloc(len, sizeof (wchar_t)); /* Allocate memory accordingly */
	debug2("win32_utf8_wide allocated %u bytes at %p", len, buf);

	if(buf != NULL)
	{
		ret = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, mbptr, -1, buf, len); /* Do conversion */
		*wptr = buf; /* Set string pointer to allocated buffer */
		if (buflen != NULL) *buflen = len * sizeof (wchar_t); /* Give length of allocated memory if needed. */

		return ret;
	}
	else
	{
		if (buflen != NULL) *buflen = 0;

		return 0;
	}
}
#endif

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
#include <sys/stat.h>

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

/* Always add a default permission mask in case of flags|O_CREAT. */
int compat_open(const char *filename, int flags)
{
	int ret;
#if defined (WANT_WIN32_UNICODE)
	wchar_t *frag = NULL;

	ret = win32_utf8_wide(filename, &frag, NULL);
	/* Fallback to plain open when ucs-2 conversion fails */
	if((frag == NULL) || (ret == 0))
		goto open_fallback;

	/*Try _wopen */
	ret = _wopen(frag, flags|_O_BINARY, _S_IREAD | _S_IWRITE);
	if(ret != -1 )
		goto open_ok; /* msdn says -1 means failure */

open_fallback:
#endif

#if (defined(WIN32) && !defined (__CYGWIN__))
	/* MSDN says POSIX function is deprecated beginning in Visual C++ 2005 */
	/* Try plain old _open(), if it fails, do nothing */
	ret = _open(filename, flags|_O_BINARY, _S_IREAD | _S_IWRITE);
#else
	ret = open(filename, flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif

#if defined (WANT_WIN32_UNICODE)
open_ok:
	/* A cast to void*? Does Windows need that?! */
	free((void *)frag);
#endif

	return ret;
}

/* Moved over from wav.c, logic with fallbacks added from the
   example of compat_open(). */
FILE* compat_fopen(const char *filename, const char *mode)
{
	FILE* stream = NULL;
#ifdef WANT_WIN32_UNICODE
	int cnt = 0;
	wchar_t *wname = NULL;
	wchar_t *wmode = NULL;

	cnt = win32_utf8_wide(filename, &wname, NULL);
	if( (wname == NULL) || (cnt == 0))
		goto fopen_fallback;
	cnt = win32_utf8_wide(mode, &wmode, NULL);
	if( (wmode == NULL) || (cnt == 0))
		goto fopen_fallback;

	stream = _wfopen(wname, wmode);
	if(stream) goto fopen_ok;

fopen_fallback:
#endif
	stream = fopen(filename, mode);
#ifdef WANT_WIN32_UNICODE

fopen_ok:
	free(wmode);
	free(wname);
#endif
	return stream;
}

int compat_close(int infd)
{
#if (defined(WIN32) && !defined (__CYGWIN__)) /* MSDN says POSIX function is deprecated beginning in Visual C++ 2005 */
	return _close(infd);
#else
	return close(infd);
#endif
}

int compat_fclose(FILE *stream)
{
	return fclose(stream);
}

/* Windows Unicode stuff */

#ifdef WANT_WIN32_UNICODE
int win32_wide_utf8(const wchar_t * const wptr, char **mbptr, size_t * buflen)
{
  size_t len;
  char *buf;
  int ret = 0;

  len = WideCharToMultiByte(CP_UTF8, 0, wptr, -1, NULL, 0, NULL, NULL); /* Get utf-8 string length */
  buf = calloc(len + 1, sizeof (char)); /* Can we assume sizeof char always = 1? */

  if(!buf) len = 0;
  else {
    if (len != 0) ret = WideCharToMultiByte(CP_UTF8, 0, wptr, -1, buf, len, NULL, NULL); /*Do actual conversion*/
    buf[len] = '0'; /* Must terminate */
  }
  *mbptr = buf; /* Set string pointer to allocated buffer */
  if(buflen != NULL) *buflen = (len) * sizeof (char); /* Give length of allocated memory if needed. */
  return ret;
}

int win32_utf8_wide(const char *const mbptr, wchar_t **wptr, size_t *buflen)
{
  size_t len;
  wchar_t *buf;
  int ret = 0;

  len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mbptr, -1, NULL, 0); /* Get converted size */
  buf = calloc(len + 1, sizeof (wchar_t)); /* Allocate memory accordingly */

  if(!buf) len = 0;
  else {
    if (len != 0) ret = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, mbptr, -1, buf, len); /* Do conversion */
    buf[len] = L'0'; /* Must terminate */
  }
  *wptr = buf; /* Set string pointer to allocated buffer */
  if (buflen != NULL) *buflen = len * sizeof (wchar_t); /* Give length of allocated memory if needed. */
  return ret; /* Number of characters written */
}
#endif


/* This shall survive signals and any return value less than given byte count
   is an error */
size_t unintr_write(int fd, void const *buffer, size_t bytes)
{
	size_t written = 0;
	while(bytes)
	{
		ssize_t part = write(fd, (char*)buffer+written, bytes);
		if(part < 0 && errno != EINTR)
			break;
		bytes   -= part;
		written += part;
	}
	return written;
}

/* Same for reading the data. */
size_t unintr_read(int fd, void *buffer, size_t bytes)
{
	size_t got = 0;
	while(bytes)
	{
		ssize_t part = read(fd, (char*)buffer+got, bytes);
		if(part < 0 && errno != EINTR)
			break;
		bytes -= part;
		got   += part;
	}
	return got;
}

#ifndef NO_CATCHSIGNAL
#if (!defined(WIN32) || defined (__CYGWIN__)) && defined(HAVE_SIGNAL_H)
void (*catchsignal(int signum, void(*handler)()))()
{
	struct sigaction new_sa;
	struct sigaction old_sa;

#ifdef DONT_CATCH_SIGNALS
	fprintf (stderr, "Not catching any signals.\n");
	return ((void (*)()) -1);
#endif

	new_sa.sa_handler = handler;
	sigemptyset(&new_sa.sa_mask);
	new_sa.sa_flags = 0;
	if(sigaction(signum, &new_sa, &old_sa) == -1)
		return ((void (*)()) -1);
	return (old_sa.sa_handler);
}
#endif
#endif

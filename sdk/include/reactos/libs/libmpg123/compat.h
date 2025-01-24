/*
	compat: Some compatibility functions and header inclusions.
	Basic standard C stuff, that may barely be above/around C89.

	The mpg123 code is determined to keep it's legacy. A legacy of old, old UNIX.
	It is envisioned to include this compat header instead of any of the "standard" headers, to catch compatibility issues.
	So, don't include stdlib.h or string.h ... include compat.h.

	copyright 2007-8 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#ifndef MPG123_COMPAT_H
#define MPG123_COMPAT_H

#include "config.h"
#include "intsym.h"

/* Disable inline for non-C99 compilers. */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#ifndef inline
#define inline
#endif
#endif

#include <errno.h>

#ifdef HAVE_STDLIB_H
/* realloc, size_t */
#include <stdlib.h>
#endif

#include        <stdio.h>
#include        <math.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Types, types, types. */
/* Do we actually need these two in addition to sys/types.h? As replacement? */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
/* We want SIZE_MAX, etc. */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
#ifndef SSIZE_MAX
#define SSIZE_MAX ((size_t)-1/2)
#endif
#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)-1)
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef OS2
#include <float.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
/* For select(), I need select.h according to POSIX 2001, else: sys/time.h sys/types.h unistd.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* compat_open makes little sense without */
#include <fcntl.h>

/* To parse big numbers... */
#ifdef HAVE_ATOLL
#define atobigint atoll
#else
#define atobigint atol
#endif

typedef unsigned char byte;

#ifndef __REACTOS__
#if defined(_MSC_VER) && !defined(MPG123_DEF_SSIZE_T)
#define MPG123_DEF_SSIZE_T
#include <stddef.h>
typedef ptrdiff_t ssize_t;
#endif
#endif /* __REACTOS__ */

/* A safe realloc also for very old systems where realloc(NULL, size) returns NULL. */
void *safe_realloc(void *ptr, size_t size);
#ifndef HAVE_STRERROR
const char *strerror(int errnum);
#endif

/* Roll our own strdup() that does not depend on libc feature test macros
   and returns NULL on NULL input instead of crashing. */
char* compat_strdup(const char *s);

/* If we have the size checks enabled, try to derive some sane printfs.
   Simple start: Use max integer type and format if long is not big enough.
   I am hesitating to use %ll without making sure that it's there... */
#if (defined SIZEOF_OFF_T) && (SIZEOF_OFF_T > SIZEOF_LONG) && (defined PRIiMAX)
# define OFF_P PRIiMAX
typedef intmax_t off_p;
#else
# define OFF_P "li"
typedef long off_p;
#endif

#if (defined SIZEOF_SIZE_T) && (SIZEOF_SIZE_T > SIZEOF_LONG) && (defined PRIuMAX)
# define SIZE_P PRIuMAX
typedef uintmax_t size_p;
#else
# define SIZE_P "lu"
typedef unsigned long size_p;
#endif

#if (defined SIZEOF_SSIZE_T) && (SIZEOF_SSIZE_T > SIZEOF_LONG) && (defined PRIiMAX)
# define SSIZE_P PRIuMAX
typedef intmax_t ssize_p;
#else
# define SSIZE_P "li"
typedef long ssize_p;
#endif

/* Get an environment variable, possibly converted to UTF-8 from wide string.
   The return value is a copy that you shall free. */
char *compat_getenv(const char* name);

/**
 * Opening a file handle can be different.
 * This function here is defined to take a path in native encoding (ISO8859 / UTF-8 / ...), or, when MS Windows Unicode support is enabled, an UTF-8 string that will be converted back to native UCS-2 (wide character) before calling the system's open function.
 * @param[in] wptr Pointer to wide string.
 * @param[in] mbptr Pointer to multibyte string.
 * @return file descriptor (>=0) or error code.
 */
int compat_open(const char *filename, int flags);
FILE* compat_fopen(const char *filename, const char *mode);
/**
 * Also fdopen to avoid having to define POSIX macros in various source files.
 */
FILE* compat_fdopen(int fd, const char *mode);

/**
 * Closing a file handle can be platform specific.
 * This function takes a file descriptor that is to be closed.
 * @param[in] infd File descriptor to be closed.
 * @return 0 if the file was successfully closed. A return value of -1 indicates an error.
 */
int compat_close(int infd);
int compat_fclose(FILE* stream);

/* Those do make sense in a separate file, but I chose to include them in compat.c because that's the one source whose object is shared between mpg123 and libmpg123 -- and both need the functionality internally. */

#ifdef WANT_WIN32_UNICODE
/**
 * win32_uni2mbc
 * Converts a null terminated UCS-2 string to a multibyte (UTF-8) equivalent.
 * Caller is supposed to free allocated buffer.
 * @param[in] wptr Pointer to wide string.
 * @param[out] mbptr Pointer to multibyte string.
 * @param[out] buflen Optional parameter for length of allocated buffer.
 * @return status of WideCharToMultiByte conversion.
 *
 * WideCharToMultiByte - https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
 */
int win32_wide_utf8(const wchar_t * const wptr, char **mbptr, size_t * buflen);

/**
 * win32_mbc2uni
 * Converts a null terminated UTF-8 string to a UCS-2 equivalent.
 * Caller is supposed to free allocated buffer.
 * @param[out] mbptr Pointer to multibyte string.
 * @param[in] wptr Pointer to wide string.
 * @param[out] buflen Optional parameter for length of allocated buffer.
 * @return status of WideCharToMultiByte conversion.
 *
 * MultiByteToWideChar - https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
 */

int win32_utf8_wide(const char *const mbptr, wchar_t **wptr, size_t *buflen);
#endif

/*
	A little bit of path abstraction: We always work with plain char strings
	that usually represent POSIX-ish UTF-8 paths (something like c:/some/file
	might appear). For Windows, those are converted to wide strings with \
	instead of / and possible fun is had with prefixes to get around the old
	path length limit. Outside of the compat library, that stuff should not
	matter, although something like //?/UNC/server/some/file could be thrown
	around as UTF-8 string, to be converted to a wide \\?\UNC\server\some\file
	just before handing it to Windows API.

	There is a lot of unnecessary memory allocation and string copying because
	of this, but this filesystem stuff is not really relevant to mpg123
	performance, so the goal is to keep the code outside the compatibility layer
	simple.
*/

/*
	Concatenate a prefix and a path, one of them alowed to be NULL.
	If the path is already absolute, the prefix is ignored. Relative
	parts (like /..) are resolved if this is sensible for the platform
	(meaning: for Windows), else they are preserved (on POSIX, actual
	file system access would be needed because of symlinks).
*/
char* compat_catpath(const char *prefix, const char* path);

/* Return 1 if the given path indicates an existing directory,
   0 otherwise. */
int compat_isdir(const char *path);

/*
	Directory traversal. This talks ASCII/UTF-8 paths externally, converts
	to/from wchar_t internally if the platform wants that. Returning NULL
	means failure to open/end of listing.
	There is no promise about sorting entries.
*/
struct compat_dir;
/* Returns NULL if either directory failed to open or listing is empty.
   Listing can still be empty even if non-NULL, so always rely on the
   nextfile/nextdir functions. */
struct compat_dir* compat_diropen(char *path);
void               compat_dirclose(struct compat_dir*);
/* Get the next entry that is a file (or symlink to one).
   The returned string is a copy that needs to be freed after use. */
char* compat_nextfile(struct compat_dir*);
/* Get the next entry that is a directory (or symlink to one).
   The returned string is a copy that needs to be freed after use. */
char* compat_nextdir (struct compat_dir*);

#ifdef USE_MODULES
/*
	For keeping the path mess local, a system-specific dlopen() variant
	is contained in here, too. This is very thin wrapping, even sparing
	definition of a handle type, just using void pointers.
	Use of absolute paths is a good idea if you want to be sure which
	file is openend, as default search paths vary.
*/
void *compat_dlopen (const char *path);
void *compat_dlsym  (void *handle, const char* name);
void  compat_dlclose(void *handle);
#endif

/* Blocking write/read of data with signal resilience.
   They continue after being interrupted by signals and always return the
   amount of processed data (shortage indicating actual problem or EOF). */
size_t unintr_write(int fd, void const *buffer, size_t bytes);
size_t unintr_read (int fd, void *buffer, size_t bytes);
size_t unintr_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* That one comes from Tellie on OS/2, needed in resolver. */
#ifdef __KLIBC__
typedef int socklen_t;
#endif

/* OSX SDK defines an enum with "normal" as value. That clashes with
   optimize.h */
#ifdef __APPLE__
#define normal mpg123_normal
#endif

#include "true.h"

#if (!defined(WIN32) || defined (__CYGWIN__)) && defined(HAVE_SIGNAL_H)
void (*catchsignal(int signum, void(*handler)()))();
#endif

#endif

/*
 * direct.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Functions for manipulating paths and directories (included from io.h)
 * plus functions for setting the current drive.
 *
 */
#ifndef	_DIRECT_H_
#define	_DIRECT_H_

/* All the headers include this file. */
#include <_mingw.h>

#define __need_wchar_t
#ifndef RC_INVOKED
#include <stddef.h>
#endif	/* Not RC_INVOKED */

#include <io.h>

#ifndef RC_INVOKED

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _DISKFREE_T_DEFINED
/* needed by _getdiskfree (also in dos.h) */
struct _diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
};
#define _DISKFREE_T_DEFINED
#endif  

/*
 * You really shouldn't be using these. Use the Win32 API functions instead.
 * However, it does make it easier to port older code.
 */
_CRTIMP int __cdecl _getdrive (void);
_CRTIMP unsigned long __cdecl _getdrives(void);
_CRTIMP int __cdecl _chdrive (int);
_CRTIMP char* __cdecl _getdcwd (int, char*, int);
_CRTIMP unsigned __cdecl _getdiskfree (unsigned, struct _diskfree_t *);

#ifndef	_NO_OLDNAMES
# define diskfree_t _diskfree_t
#endif

#ifndef _WDIRECT_DEFINED
/* wide character versions. Also in wchar.h */
#ifdef __MSVCRT__ 
_CRTIMP int __cdecl _wchdir(const wchar_t*);
_CRTIMP wchar_t* __cdecl _wgetcwd(wchar_t*, int);
_CRTIMP wchar_t* __cdecl _wgetdcwd(int, wchar_t*, int);
_CRTIMP int __cdecl _wmkdir(const wchar_t*);
_CRTIMP int __cdecl _wrmdir(const wchar_t*);
#endif	/* __MSVCRT__ */
#define _WDIRECT_DEFINED
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _DIRECT_H_ */

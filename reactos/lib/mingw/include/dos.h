/*
 * dos.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * DOS-specific functions and structures.
 *
 */

#ifndef	_DOS_H_
#define	_DOS_H_

/* All the headers include this file. */
#include <_mingw.h>

#define __need_wchar_t
#ifndef RC_INVOKED
#include <stddef.h>
#endif	/* Not RC_INVOKED */

/* For DOS file attributes */
#include <io.h>

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MSVCRT__ /* these are in CRTDLL, but not MSVCRT */
#ifndef __DECLSPEC_SUPPORTED
extern unsigned int *_imp___basemajor_dll;
extern unsigned int *_imp___baseminor_dll;
extern unsigned int *_imp___baseversion_dll;
extern unsigned int *_imp___osmajor_dll;
extern unsigned int *_imp___osminor_dll;
extern unsigned int *_imp___osmode_dll;

#define _basemajor (*_imp___basemajor_dll)
#define _baseminor (*_imp___baseminor_dll)
#define _baseversion (*_imp___baseversion_dll)
#define _osmajor (*_imp___osmajor_dll)
#define _osminor (*_imp___osminor_dll)
#define _osmode (*_imp___osmode_dll)

#else /* __DECLSPEC_SUPPORTED */

__MINGW_IMPORT unsigned int _basemajor_dll;
__MINGW_IMPORT unsigned int _baseminor_dll;
__MINGW_IMPORT unsigned int _baseversion_dll;
__MINGW_IMPORT unsigned int _osmajor_dll;
__MINGW_IMPORT unsigned int _osminor_dll;
__MINGW_IMPORT unsigned int _osmode_dll;

#define _basemajor _basemajor_dll
#define _baseminor _baseminor_dll
#define _baseversion _baseversion_dll
#define _osmajor _osmajor_dll
#define _osminor _osminor_dll
#define _osmode _osmode_dll

#endif /* __DECLSPEC_SUPPORTED */
#endif /* ! __MSVCRT__ */

#ifndef _DISKFREE_T_DEFINED
/* needed by _getdiskfree (also in direct.h) */
struct _diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
};
#define _DISKFREE_T_DEFINED
#endif  

_CRTIMP unsigned __cdecl _getdiskfree (unsigned, struct _diskfree_t *);

#ifndef	_NO_OLDNAMES
# define diskfree_t _diskfree_t
#endif

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _DOS_H_ */

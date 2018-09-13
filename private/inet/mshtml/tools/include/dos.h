/***
*dos.h - definitions for MS-DOS interface routines
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines the structs and unions used for the direct DOS interface
*	routines; includes macros to access the segment and offset
*	values of far pointers, so that they may be used by the routines; and
*	provides function prototypes for direct DOS interface functions.
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_DOS
#define _INC_DOS

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef	_MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif	/* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

#ifndef _MAC
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif /* ndef _MAC */

#ifndef _MAC
#ifndef _DISKFREE_T_DEFINED
/* _getdiskfree structure (duplicated in DIRECT.H) */
struct _diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
	};

#define _DISKFREE_T_DEFINED
#endif
#endif /* ndef _MAC */

/* File attribute constants */

#define _A_NORMAL	0x00	/* Normal file - No read/write restrictions */
#define _A_RDONLY	0x01	/* Read only file */
#define _A_HIDDEN	0x02	/* Hidden file */
#define _A_SYSTEM	0x04	/* System file */
#define _A_SUBDIR	0x10	/* Subdirectory */
#define _A_ARCH 	0x20	/* Archive file */

#ifdef	_NTSDK

/* External variable declarations */

/*
 * WARNING! The _osversion, _osmajor, _osminor, _baseversion, _basemajor and
 * _baseminor variables were never meaningfully defined in the C runtime
 * libraries for Win32 platforms. Any code which references these variables
 * should be revised (see the declarations for version information variables
 * in stdlib.h).
 */

#ifdef	_DLL

/* --------- The following block is OBSOLETE --------- */

#define _osversion   (*_osversion_dll)
#define _osmajor     (*_osmajor_dll)
#define _osminor     (*_osminor_dll)
#define _baseversion (*_baseversion_dll)
#define _basemajor   (*_basemajor_dll)
#define _baseminor   (*_baseminor_dll)

extern unsigned int * _osversion_dll;
extern unsigned int * _osmajor_dll;
extern unsigned int * _osminor_dll;
extern unsigned int * _baseversion_dll;
extern unsigned int * _basemajor_dll;
extern unsigned int * _baseminor_dll;

/* --------- The preceding block is OBSOLETE --------- */

#define _pgmptr      (*_pgmptr_dll)
extern char ** _pgmptr_dll;

#ifndef _MAC
#define _wpgmptr     (*_wpgmptr_dll)
extern wchar_t ** _wpgmptr_dll;
#endif /* ndef _MAC */

#else	/* ndef _DLL */

/* --------- The following block is OBSOLETE --------- */


extern unsigned int _osversion;
extern unsigned int _osmajor;
extern unsigned int _osminor;
extern unsigned int _baseversion;
extern unsigned int _basemajor;
extern unsigned int _baseminor;

/* --------- The preceding block is OBSOLETE --------- */


extern char * _pgmptr;
#ifndef _MAC
extern wchar_t * _wpgmptr;
#endif /* ndef _MAC */

#endif	/* _DLL */

#endif	/* _NTSDK */

#ifndef _MAC
/* Function prototypes */
_CRTIMP unsigned __cdecl _getdiskfree(unsigned, struct _diskfree_t *);
#endif /* ndef _MAC */

#ifdef	_M_IX86
void __cdecl _disable(void);
void __cdecl _enable(void);
#endif	/* _M_IX86 */

#ifndef _MAC
#if	!__STDC__
/* Non-ANSI name for compatibility */
#define diskfree_t  _diskfree_t
#endif	/* __STDC__ */
#endif /* ndef _MAC */

#ifdef __cplusplus
}
#endif

#ifdef	_MSC_VER
#pragma pack(pop)
#endif	/* _MSC_VER */

#endif	/* _INC_DOS */

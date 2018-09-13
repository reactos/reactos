/**		types.h - Generic types
 *
 *		This file contains generic types such as USHORT, ushort,
 *		WORD, etc., which are not directly related to CodeView.
 *		Every attempt is made to define them in such a way as they
 *		will not conflict with the standard header files such as
 *		windows.h and os2.h.
 */


/***	The master copy of this file resides in the CVINC project.
 *		All Microsoft projects are required to use the master copy without
 *		modification.  Modification of the master version or a copy
 *		without consultation with all parties concerned is extremely
 *		risky.
 *
 *		The projects known to use this file are:
 *
 *			CodeView (uses version in CVINC project)
 *			C/C++ expression evaluator (uses version in CVINC project)
 *			Symbol Handler (uses version in CVINC project)
 *			Stump (OSDebug) (uses version in CVINC project)
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CVINC_TYPES /* whole file */
#define CVINC_TYPES

#ifdef HOST32	/* { */

#define _export

#ifndef LOADDS
#define	LOADDS
#endif

#ifndef PASCAL
#define PASCAL __stdcall
#endif

#ifndef CDECL
#define	CDECL _cdecl
#endif

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#ifndef far
#define far
#endif

#ifndef FAR
#define	FAR far
#endif

#ifndef near
#define near
#endif

#ifndef NEAR
#define NEAR near
#endif

#ifndef _HUGE_
#define _HUGE_
#endif

// Use SEGBASED for based on a segment [e.g. SEGBASED(__segname("_CODE"))]
#ifndef SEGBASED
#define SEGBASED(x)
#endif

/*
**	This set of functions need to be expanded to deal with
**	unicode and other problems.
*/

// These #defines are taken care of by windowsx.h
// (WinslowF) map to _tcs functions

#ifndef _INC_WINDOWSX

#define _ncalloc    calloc
#define _nexpand    expand
#define _ffree      free
#define _fmalloc    malloc
#define _fmemccpy   _memccpy
#define _fmemchr    memchr
#define _fmemcmp    memcmp
#define _fmemcpy    memcpy
#define _fmemicmp   _memicmp
#define _fmemmove   memmove
#define _fmemset    memset
#define _fmsize     _msize
#define _frealloc   realloc
#define _fstrcat    _tcscat
#define _fstrchr    _tcschr
#define _fstrcmp    _tcscmp
#define _fstrcpy    _tcscpy
#define _fstrcspn   _tcscspn
#define _fstrdup    _tcsdup
#define _fstricmp   _tcsicmp
#define _fstrlen    _tcslen
#define _fstrlwr    _tcslwr
#define _fstrncat   _tcsncat
#define _fstrncmp   _tcsncmp
#define _fstrncpy   _tcsncpy
#define _fstrnicmp  _tcsnicmp
#define _fstrnset   _tcsnset
#define _fstrpbrk   _tcspbrk
#define _fstrrchr   _tcsrchr
#define _fstrrev    _tcsrev
#define _fstrset    _tcsset
#define _fstrspn    _tcsspn
#define _fstrstr    _tcsstr
#define _fstrtok    _tcstok
#define _fstrupr    _tcsupr
#define _nfree      free
#define _nmalloc    malloc
#define _nmsize     msize
#define _nrealloc   realloc
#define _nstrdup    _strdup
#define hmemcpy     memcpy

#endif

#define FP_OFF(x) x

#else	// !HOST32 }{

#ifndef LOADDS
#define LOADDS _loadds
#endif

#ifndef PASCAL
#define PASCAL _pascal
#endif

#ifndef CDECL
#define	CDECL _cdecl
#endif

#ifndef FASTCALL
#define FASTCALL _fastcall
#endif

#ifndef FAR
#define FAR _far
#endif

#ifndef NEAR
#define NEAR _near
#endif

#ifndef _HUGE_
#define _HUGE_ _huge
#endif

// Use SEGBASED for based on a segment [e.g. SEGBASED(__segname("_CODE"))]
#ifndef SEGBASED
#define SEGBASED(x) _based(x)
#endif

#endif	// HOST32 }

#ifndef INTERRUPT
#define INTERRUPT _interrupt
#endif

#ifndef LOCAL
#ifdef DEBUGVER
#define LOCAL
#else
#define LOCAL static
#endif
#endif

#ifndef GLOBAL
#define GLOBAL
#endif

#ifndef INLINE
#define INLINE __inline
#endif

//
// Things that come from either windows.h or os2.h
//

#if !defined(LOWORD) && !defined(OS2_INCLUDED)

	#define VOID			void

	typedef unsigned char	BYTE;

	typedef int				BOOL;

	#define LONG			long

#endif

//
// Things that come from windows.h and cwindows.h
//

#if !defined(LOWORD)

#if defined ( _WIN32 ) || defined ( _M_MPPC )
	typedef void *			HANDLE;
#else
	typedef unsigned int	HANDLE;
#endif

	typedef HANDLE			HWND;
	typedef char FAR *		LPSTR;

	typedef unsigned short	WORD;
	typedef unsigned long	DWORD;

	#define WNDPROC			FARPROC

#endif

#if !defined ( WIN32 ) && !defined ( WIN32S ) && !defined ( WIN )

    typedef unsigned long       DWORD;
    typedef int                 BOOL;
    typedef unsigned char       BYTE;
    typedef unsigned short      WORD;
    typedef float               FLOAT;
    typedef FLOAT               *PFLOAT;
    typedef BOOL near           *PBOOL;
    typedef BOOL far            *LPBOOL;
    typedef BYTE near           *PBYTE;
    typedef BYTE far            *LPBYTE;
    typedef int near            *PINT;
    typedef int far             *LPINT;
    typedef WORD near           *PWORD;
    typedef WORD far            *LPWORD;
    typedef long far            *LPLONG;
    typedef DWORD near          *PDWORD;
    typedef DWORD far           *LPDWORD;
    typedef void far            *LPVOID;

    typedef int                 INT;
    typedef unsigned int        UINT;
    typedef unsigned int        *PUINT;

    typedef HANDLE FAR          *LPHANDLE;

#endif


//
// Things that come from os2.h
//

#if !defined(OS2_INCLUDED)

	#define CHAR			char

	typedef	unsigned char	UCHAR;
	typedef short			SHORT;
	typedef int				INT;
	typedef unsigned short	USHORT;
	typedef unsigned int	UINT;
	typedef unsigned long	ULONG;

	typedef char *			PCH;

#endif

#if !defined(LOWORD)

	#define LOWORD(l)	((WORD)(l))
	#define HIWORD(l)	((WORD)(((DWORD)(l) >> 16) & 0xFFFF))

#endif

#ifndef NULL
	#define	NULL		((void *) 0)
#endif

#if !defined(TRUE) || !defined(FALSE)
	#undef TRUE
	#undef FALSE

	#define FALSE		0
	#define TRUE		1
#endif

#if !defined(fTrue) || !defined(fFalse)
	#undef fTrue
	#undef fFalse

	#define fFalse		0
	#define fTrue		1
#endif

#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef Unreferenced
#define	Unreferenced(a) ((void)a)
#endif

typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned long  ulong;
typedef unsigned int   uint;

typedef void *		PV;
typedef void FAR *	LPV;

typedef char *		SZ;
typedef char FAR *	LSZ;
typedef char FAR *	LPCH;

typedef BOOL FAR *	LPF;
typedef BYTE FAR *	LPB;
typedef WORD FAR *  LPW;
typedef DWORD FAR * LPDW;
typedef LONG FAR *	LPL;
typedef ULONG FAR *	LPUL;
typedef USHORT FAR *LPUS;
typedef DWORD FAR *	LPDWORD;

typedef short		SWORD;

#ifndef _BASETSD_H_
typedef UINT		WPARAM;
typedef LONG		LPARAM;
#endif

#ifdef HOST32
typedef ULONG		IWORD;
#else
typedef USHORT		IWORD;
#endif

#ifdef __cplusplus
} // extern "C" {
#endif

#endif /* CVINC_TYPES */

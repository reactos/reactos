/***
*conio.h - console and port I/O declarations
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This include file contains the function declarations for
*	the MS C V2.03 compatible console I/O routines.
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_CONIO
#define _INC_CONIO

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifndef _MAC

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


/* Function prototypes */

_CRTIMP char * __cdecl _cgets(char *);
_CRTIMP int __cdecl _cprintf(const char *, ...);
_CRTIMP int __cdecl _cputs(const char *);
_CRTIMP int __cdecl _cscanf(const char *, ...);
_CRTIMP int __cdecl _getch(void);
_CRTIMP int __cdecl _getche(void);
#ifdef	_M_IX86
int __cdecl _inp(unsigned short);
unsigned short __cdecl _inpw(unsigned short);
unsigned long __cdecl _inpd(unsigned short);
#endif	/* _M_IX86 */
_CRTIMP int __cdecl _kbhit(void);
#ifdef	_M_IX86
int __cdecl _outp(unsigned short, int);
unsigned short __cdecl _outpw(unsigned short, unsigned short);
unsigned long __cdecl _outpd(unsigned short, unsigned long);
#endif	/* _M_IX86 */
_CRTIMP int __cdecl _putch(int);
_CRTIMP int __cdecl _ungetch(int);


#if	!__STDC__

/* Non-ANSI names for compatibility */

#ifdef	_NTSDK

#define cgets	_cgets
#define cprintf _cprintf
#define cputs	_cputs
#define cscanf	_cscanf
#define getch	_getch
#define getche	_getche
#define kbhit	_kbhit
#define putch	_putch
#define ungetch _ungetch

#else	/* ndef _NTSDK */

_CRTIMP char * __cdecl cgets(char *);
_CRTIMP int __cdecl cprintf(const char *, ...);
_CRTIMP int __cdecl cputs(const char *);
_CRTIMP int __cdecl cscanf(const char *, ...);
#ifdef	_M_IX86
int __cdecl inp(unsigned short);
unsigned short __cdecl inpw(unsigned short);
#endif	/* _M_IX86 */
_CRTIMP int __cdecl getch(void);
_CRTIMP int __cdecl getche(void);
_CRTIMP int __cdecl kbhit(void);
#ifdef	_M_IX86
int __cdecl outp(unsigned short, int);
unsigned short __cdecl outpw(unsigned short, unsigned short);
#endif	/* _M_IX86 */
_CRTIMP int __cdecl putch(int);
_CRTIMP int __cdecl ungetch(int);

#endif	/* _NTSDK */

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _MAC */

#endif	/* _INC_CONIO */

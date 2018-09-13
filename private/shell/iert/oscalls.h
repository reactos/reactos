/***
*oscalls.h - contains declarations of Operating System types and constants.
*
*	Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Declares types and constants that are defined by the target OS.
*
*	[Internal]
*
*Revision History:
*	12-01-90  SRW	Module created
*	02-01-91  SRW	Removed usage of NT header files (_WIN32)
*	02-28-91  SRW	Removed usage of ntconapi.h (_WIN32)
*	04-09-91  PNT   Added _MAC_ definitions
*	04-26-91  SRW   Disable min/max definitions in windows.h and added debug
*			definitions for DbgPrint and DbgBreakPoint(_WIN32)
*	08-05-91  GJF	Use win32.h instead of windows.h for now.
*	08-20-91  JCR	C++ and ANSI naming
*	09-12-91  GJF	Go back to using windows.h for win32 build.
*	09-26-91  GJF	Don't use error.h for Win32.
*	11-07-91  GJF	win32.h renamed to dosx32.h
*	11-08-91  GJF	Don't use windows.h, excpt.h. Add ntstatus.h.
*	12-13-91  GJF	Fixed so that exception stuff will build for Win32
*	02-04-92  GJF	Now must include ntdef.h to get LPSTR type.
*	02-07-92  GJF	Backed out change above, LPSTR also got added to
*			winnt.h
*	03-30-92  DJM	POSIX support.
*	04-06-92  SRW	Backed out 11-08-91 change and went back to using
*                       windows.h only.
*	05-12-92  DJM	Moved POSIX code to it's own ifdef.
*	08-01-92  SRW	Let windows.h include excpt.h now that it replaces winxcpt.h
*	09-30-92  SRW   Use windows.h for _POSIX_ as well
*	02-23-93  SKS	Update copyright to 1993
*	09-06-94  CFW	Remove Cruiser support.
*	02-06-95  CFW	DEBUG -> _DEBUG
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*
****/

#if _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_OSCALLS
#define _INC_OSCALLS

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#ifdef NULL
#undef NULL
#endif

#if defined(_DEBUG) && defined(_WIN32)

void DbgBreakPoint(void);
int DbgPrint(char *Format, ...);

#endif	/* _DEBUG && _WIN32 */

#define NOMINMAX

#include <windows.h>

#undef NULL
#ifndef NULL
#ifdef __cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif
#endif

/* File time and date types */

typedef struct _FTIME {         /* ftime */
    unsigned short twosecs : 5;
    unsigned short minutes : 6;
    unsigned short hours   : 5;
} FTIME;
typedef FTIME	*PFTIME;

typedef struct _FDATE {         /* fdate */
    unsigned short day	   : 5;
    unsigned short month   : 4;
    unsigned short year    : 7;
} FDATE;
typedef FDATE	*PFDATE;

#else	/* ndef _WIN32 */

#ifdef _POSIX_

#undef NULL
#ifdef __cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif

#include <windows.h>

#else   /* ndef _POSIX_ */

#if	defined(_M_M68K) || defined(_M_MPPC)

#else	/* ndef defined(_M_M68K) || defined(_M_MPPC) */

#error ERROR - ONLY WIN32, POSIX, OR MAC TARGET SUPPORTED!

#endif  /* _POSIX_ */

#endif	/* defined(_M_M68K) || defined(_M_MPPC) */

#endif	/* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif	/* _INC_OSCALLS */

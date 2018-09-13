/***
*oscalls.h - contains declarations of Operating System types and constants.
*
* Copyright (c) 1985 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       Declares types and constants that are defined by the target OS.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_OSCALLS
#define _INC_OSCALLS

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef _WIN32

#ifdef NULL
#undef NULL
#endif  /* NULL */

#if defined (_DEBUG) && defined (_WIN32)

void DbgBreakPoint(void);
int DbgPrint(char *Format, ...);

#endif  /* defined (_DEBUG) && defined (_WIN32) */

#define NOMINMAX

#include <windows.h>

#undef NULL
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

/* File time and date types */

typedef struct _FTIME {         /* ftime */
    unsigned short twosecs : 5;
    unsigned short minutes : 6;
    unsigned short hours   : 5;
} FTIME;
typedef FTIME   *PFTIME;

typedef struct _FDATE {         /* fdate */
    unsigned short day     : 5;
    unsigned short month   : 4;
    unsigned short year    : 7;
} FDATE;
typedef FDATE   *PFDATE;

#else  /* _WIN32 */


#if defined (_M_M68K) || defined (_M_MPPC)

#else  /* defined (_M_M68K) || defined (_M_MPPC) */

#error ERROR - ONLY WIN32, POSIX, OR MAC TARGET SUPPORTED!

#endif  /* defined (_M_M68K) || defined (_M_MPPC) */


#endif  /* _WIN32 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_OSCALLS */

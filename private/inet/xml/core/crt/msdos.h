/***
*msdos.h - MS-DOS definitions for C runtime
*
* Copyright (c) 1987 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       The file contains the MS-DOS definitions (function request numbers,
*       flags, etc.) used by the C runtime.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_MSDOS
#define _INC_MSDOS

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

/* Stack slop for o.s. system call overhead */

#define _STACKSLOP      1024

/* __osfile flag values for DOS file handles */

#define FOPEN           0x01    /* file handle open */
#define FEOFLAG         0x02    /* end of file has been encountered */
#if defined (_M_M68K) || defined (_M_MPPC)
#define FWRONLY         0x04    /* file handle associated with write only file */
#define FLOCK           0x08    /* file has been successfully locked at least once */
#else  /* defined (_M_M68K) || defined (_M_MPPC) */
#define FCRLF           0x04    /* CR-LF across read buffer (in text mode) */
#define FPIPE           0x08    /* file handle refers to a pipe */
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */
#ifdef _WIN32
#define FNOINHERIT      0x10    /* file handle opened _O_NOINHERIT */
#else  /* _WIN32 */
#define FRDONLY         0x10    /* file handle associated with read only file */
#endif  /* _WIN32 */

#define FAPPEND         0x20    /* file handle opened O_APPEND */
#define FDEV            0x40    /* file handle refers to device */
#define FTEXT           0x80    /* file handle is in text mode */

/* DOS errno values for setting __doserrno in C routines */

#define E_ifunc         1       /* invalid function code */
#define E_nofile        2       /* file not found */
#define E_nopath        3       /* path not found */
#define E_toomany       4       /* too many open files */
#define E_access        5       /* access denied */
#define E_ihandle       6       /* invalid handle */
#define E_arena         7       /* arena trashed */
#define E_nomem         8       /* not enough memory */
#define E_iblock        9       /* invalid block */
#define E_badenv        10      /* bad environment */
#define E_badfmt        11      /* bad format */
#define E_iaccess       12      /* invalid access code */
#define E_idata         13      /* invalid data */
#define E_unknown       14      /* ??? unknown error ??? */
#define E_idrive        15      /* invalid drive */
#define E_curdir        16      /* current directory */
#define E_difdev        17      /* not same device */
#define E_nomore        18      /* no more files */
#define E_maxerr2       19      /* unknown error - Version 2.0 */
#define E_sharerr       32      /* sharing violation */
#define E_lockerr       33      /* locking violation */
#define E_maxerr3       34      /* unknown error - Version 3.0 */

/* DOS file attributes */

#define A_RO            0x1     /* read only */
#define A_H             0x2     /* hidden */
#define A_S             0x4     /* system */
#define A_V             0x8     /* volume id */
#define A_D             0x10    /* directory */
#define A_A             0x20    /* archive */

#define A_MOD   (A_RO+A_H+A_S+A_A)      /* changeable attributes */

#endif  /* _INC_MSDOS */

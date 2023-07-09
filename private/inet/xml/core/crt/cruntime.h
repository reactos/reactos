/***
*cruntime.h - definitions specific to the target operating system and hardware
*
* Copyright (c) 1990 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This header file contains widely used definitions specific to the
*       host operating system and hardware. It is included by every C source
*       and most every other header file.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_CRUNTIME
#define _INC_CRUNTIME

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) || defined (_M_IA64) || defined (UNIX)
#define UNALIGNED __unaligned
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
#define UNALIGNED
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

/*
 * Does _VA_LIST_T still need to be defined in this file?
 */
#ifdef _M_ALPHA
#define _VA_LIST_T \
    struct { \
        char *a0;       /* pointer to first homed integer argument */ \
        int offset;     /* byte offset of next parameter */ \
    }
#else  /* _M_ALPHA */
#define _VA_LIST_T  char *
#endif  /* _M_ALPHA */

#ifdef _M_IX86
/*
 * 386/486
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#elif (defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC))
/*
 * MIPS, ALPHA, or PPC
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4    register
#define REG5    register
#define REG6    register
#define REG7    register
#define REG8    register
#define REG9    register

#elif (defined (_M_M68K) || defined (_M_MPPC))
/*
 * Macros defining the calling type of a function
 */

#define _CALLTYPE1      __cdecl    /* old -- check source user visible functions */
#define _CALLTYPE2      __cdecl    /* old -- check source user visible functions */
#define _CALLTYPE3      illegal    /* old -- check source should not used*/
#define _CALLTYPE4      __cdecl    /* old -- check source internal (static) functions */

/*
 * Macros for defining the naming of a public variable
 */

#define _VARTYPE1

/*
 * Macros for register variable declarations
 */

#define REG1
#define REG2
#define REG3
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#else  /* (defined (_M_M68K) || defined (_M_MPPC)) */

#ifndef UNIX
#pragma message ("Machine register set not defined")
#endif /* UNIX */

/*
 * Unknown machine
 */

#define REG1
#define REG2
#define REG3
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#endif  /* (defined (_M_M68K) || defined (_M_MPPC)) */

/*
 * Are the macro definitions below still needed in this file? Are they even
 * correct for MIPS (probably not).
 */

#endif  /* _INC_CRUNTIME */

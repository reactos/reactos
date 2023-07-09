/***
*assert.h - define the assert macro
*
*   Copyright (c) 1985-1987, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   Defines the assert(exp) macro.
*   [ANSI/System V]
*
*Modified:
*   Allent 3/14/88 - call Quit instead of abort
*******************************************************************************/
#ifndef _ASSERT_DEFINED

#ifdef DEBUGVER

#define assert(exp) { \
    if (!(exp)) { \
        LBPrintf( #exp, __FILE__, __LINE__); \
        LBQuit(2); \
    } \
}

#else

#define assert(exp)

#endif /* NDEBUG */

#define _ASSERT_DEFINED

#endif /* _ASSERT_DEFINED */

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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUGVER

static char _assertstring[] = "Assertion failed: %s, file %s, line %d\n";

#define assert(exp) { \
    if (!(exp)) { \
       AssertOut( #exp, __FILE__, __LINE__ );\
       quit(2); \
      } \
    }

#else

#define assert(exp)

#endif /* NDEBUG */

#define _ASSERT_DEFINED

#ifdef __cplusplus
} // extern "C" {
#endif

#endif /* _ASSERT_DEFINED */

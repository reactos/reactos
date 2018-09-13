/***
*assert.h - define the assert macro
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   Defines the assert(exp) macro.
*   [ANSI/System V]
*
****/

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#undef  assert

#ifdef NDEBUG

#define assert(exp) ((void)0)

#else 
#ifdef __cplusplus
extern "C" {
#endif 
void __cdecl _assert(void *, void *, unsigned);
#ifdef __cplusplus
}
#endif 

#define assert(exp) \
    ( (exp) ? (void) 0 : _assert(#exp, __FILE__, __LINE__) )

#endif 

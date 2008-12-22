/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef __ASSERT_H_
#define __ASSERT_H_

#include <crtdefs.h>

#ifdef NDEBUG

#ifndef assert
#define assert(_Expression) ((void)0)
#endif

#else /* !NDEBUG */

#ifdef __cplusplus
extern "C" {
#endif

//extern void __cdecl _wassert(const wchar_t *_Message,const wchar_t *_File,unsigned _Line);

#ifdef __cplusplus
}
#endif

#ifndef assert
#define assert(_Expression) (void)((!!(_Expression)))// || (_wassert(_CRT_WIDE(#_Expression),_CRT_WIDE(__FILE__),__LINE__),0))
#endif

#endif

#endif

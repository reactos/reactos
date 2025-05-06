/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WDIRECT_DEFINED
#define _WDIRECT_DEFINED

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int      __cdecl _wchdir(const wchar_t*);
_ACRTIMP wchar_t* __cdecl _wgetcwd(wchar_t*,int);
_ACRTIMP wchar_t* __cdecl _wgetdcwd(int,wchar_t*,int);
_ACRTIMP int      __cdecl _wmkdir(const wchar_t*);
_ACRTIMP int      __cdecl _wrmdir(const wchar_t*);

#ifdef __cplusplus
}
#endif

#endif /* _WDIRECT_DEFINED */

/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WPROCESS_DEFINED
#define _WPROCESS_DEFINED

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP intptr_t __cdecl _wexecl(const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wexecle(const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wexeclp(const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wexeclpe(const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wexecv(const wchar_t*,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wexecve(const wchar_t*,const wchar_t* const *,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wexecvp(const wchar_t*,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wexecvpe(const wchar_t*,const wchar_t* const *,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wspawnl(int,const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wspawnle(int,const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wspawnlp(int,const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wspawnlpe(int,const wchar_t*,const wchar_t*,...);
_ACRTIMP intptr_t __cdecl _wspawnv(int,const wchar_t*,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wspawnve(int,const wchar_t*,const wchar_t* const *,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wspawnvp(int,const wchar_t*,const wchar_t* const *);
_ACRTIMP intptr_t __cdecl _wspawnvpe(int,const wchar_t*,const wchar_t* const *,const wchar_t* const *);
_ACRTIMP int      __cdecl _wsystem(const wchar_t*);

#ifdef __cplusplus
}
#endif

#endif /* _WPROCESS_DEFINED */

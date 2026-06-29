/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _INC_FENV
#define _INC_FENV

#include <float.h>

#define FE_TONEAREST   _RC_NEAR
#define FE_UPWARD      _RC_UP
#define FE_DOWNWARD    _RC_DOWN
#define FE_TOWARDZERO  _RC_CHOP

#define FE_INEXACT     _SW_INEXACT
#define FE_UNDERFLOW   _SW_UNDERFLOW
#define FE_OVERFLOW    _SW_OVERFLOW
#define FE_DIVBYZERO   _SW_ZERODIVIDE
#define FE_INVALID     _SW_INVALID
#define FE_ALL_EXCEPT  (FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    __msvcrt_ulong _Fe_ctl;
    __msvcrt_ulong _Fe_stat;
} fenv_t;

typedef __msvcrt_ulong fexcept_t;

_ACRTIMP int __cdecl fegetenv(fenv_t*);
_ACRTIMP int __cdecl fesetenv(const fenv_t*);
_ACRTIMP int __cdecl fegetexceptflag(fexcept_t*, int);
_ACRTIMP int __cdecl fegetround(void);
_ACRTIMP int __cdecl feholdexcept(fenv_t*);
_ACRTIMP int __cdecl fesetround(int);
_ACRTIMP int __cdecl fesetexceptflag(const fexcept_t*, int);
_ACRTIMP int __cdecl feclearexcept(int);
_ACRTIMP int __cdecl fetestexcept(int);

#ifdef __cplusplus
}
#endif

#endif /* _INC_FENV */

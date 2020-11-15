/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#undef CRTDLL
#ifndef _DLL
#define _DLL
#endif

#include <oscalls.h>
#include <internal.h>
#include <stdlib.h>
#include <crtdefs.h>
#include <limits.h>
//#include <windows.h>

#define _EXIT_LOCK1 8

  void __cdecl _lock (int _File);
  void __cdecl _unlock (int _File);
  
_PVFV *__onexitbegin;
_PVFV *__onexitend;

extern _onexit_t __cdecl __dllonexit (_onexit_t, _PVFV**, _PVFV**);
extern _onexit_t (__cdecl * __MINGW_IMP_SYMBOL(_onexit)) (_onexit_t func);

/* INTERNAL: call atexit functions */
void __call_atexit(void)
{
    /* Note: should only be called with the exit lock held */
    _PVFV *first, *last;

    first =  (_PVFV *)_decode_pointer(__onexitbegin);
    last = (_PVFV *)_decode_pointer(__onexitend);;

    if (!first) return;

    while (--last >= first)
        if (*last)
            (**last)();

    free(first);
}

/* Choose a different name to prevent name conflicts. The CRT one works fine.  */
_onexit_t __cdecl _onexit(_onexit_t func);

_onexit_t __cdecl _onexit(_onexit_t func)
{
  _PVFV *onexitbegin;
  _PVFV *onexitend;
  _onexit_t retval;

  onexitbegin = (_PVFV *) _decode_pointer (__onexitbegin);

  if (onexitbegin == (_PVFV *) -1)
#ifdef __REACTOS__
  {
      onexitbegin = (_PVFV *)calloc(32, sizeof(_onexit_t));
      if (onexitbegin == NULL)
        return NULL;
      __onexitbegin = _encode_pointer(onexitbegin);
      __onexitend = _encode_pointer(onexitbegin + 32);
  }
#else
    return (* __MINGW_IMP_SYMBOL(_onexit)) (func);
#endif
  _lock (_EXIT_LOCK1);
  onexitbegin = (_PVFV *) _decode_pointer (__onexitbegin);
  onexitend = (_PVFV *) _decode_pointer (__onexitend);
  
  retval = __dllonexit (func, &onexitbegin, &onexitend);

  __onexitbegin = (_PVFV *) _encode_pointer (onexitbegin);
  __onexitend = (_PVFV *) _encode_pointer (onexitend);
  _unlock (_EXIT_LOCK1);
  return retval;
}

int __cdecl
atexit (_PVFV func)
{
  return (_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}

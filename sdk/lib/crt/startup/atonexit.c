/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

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

    if (!__onexitbegin)
        return;

    first =  (_PVFV *)_decode_pointer(__onexitbegin);
    last = (_PVFV *)_decode_pointer(__onexitend);

    while (--last >= first)
        if (*last)
            (**last)();

    free(first);

    __onexitbegin = __onexitend = NULL;
}

/* Choose a different name to prevent name conflicts. The CRT one works fine.  */
_onexit_t __cdecl _onexit(_onexit_t func);

_onexit_t __cdecl _onexit(_onexit_t func)
{
    _PVFV *onexitbegin;
    _PVFV *onexitend;
    _onexit_t retval;

#ifndef CRTDLL
    if (__onexitbegin == (_PVFV *) -1)
        return (* __MINGW_IMP_SYMBOL(_onexit)) (func);
#endif

    _lock (_EXIT_LOCK1);

    if (!__onexitbegin)
    {
        /* First time we are called. Initialize our array */
        onexitbegin = calloc(1, sizeof(*onexitbegin));
        if (!onexitbegin)
        {
            _unlock(_EXIT_LOCK1);
            return NULL;
        }
        onexitend = onexitbegin;
    }
    else
    {
        onexitbegin = (_PVFV *) _decode_pointer (__onexitbegin);
        onexitend = (_PVFV *) _decode_pointer (__onexitend);
    }

    retval = __dllonexit (func, &onexitbegin, &onexitend);

    if (retval != NULL)
    {
        /* Update our globals in case of success */
        __onexitbegin = (_PVFV *) _encode_pointer (onexitbegin);
        __onexitend = (_PVFV *) _encode_pointer (onexitend);
    }

    _unlock (_EXIT_LOCK1);
    return retval;
}

int __cdecl
atexit (_PVFV func)
{
  return (_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}

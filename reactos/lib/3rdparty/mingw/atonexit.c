#undef CRTDLL
#ifndef _DLL
#define _DLL
#endif

#include "oscalls.h"
#include "internal.h"
#include <stdlib.h>
#include <limits.h>
#include <windows.h>

#define _EXIT_LOCK1 8

  void __cdecl _lock (int _File);
  void __cdecl _unlock (int _File);
  
_PVFV *__onexitbegin;
_PVFV *__onexitend;

extern _CRTIMP _onexit_t __dllonexit (_onexit_t, _PVFV**, _PVFV**);
extern _onexit_t (__cdecl *_imp___onexit) (_onexit_t func);

#if 0
/* Choose a different name to prevent name conflicts. The CRT one works fine.  */
_onexit_t __cdecl mingw_onexit(_onexit_t func)
{
  _PVFV *onexitbegin;
  _PVFV *onexitend;
  _onexit_t retval;

  onexitbegin = (_PVFV *) _decode_pointer (__onexitbegin);

  if (onexitbegin == (_PVFV *) -1)
    return (*_imp___onexit) (func);
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
  return (mingw_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}
#endif

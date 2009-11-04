/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <windows.h>
#include <stdlib.h>
#include <setjmp.h>

typedef void (*func_ptr) (void);
extern func_ptr __CTOR_LIST__[];
extern func_ptr __DTOR_LIST__[];

static HMODULE hMsvcrt = NULL;
static int free_Msvcrt = 0;

typedef void __cdecl flongjmp(jmp_buf _Buf,int _Value);

flongjmp *fctMsvcrtLongJmp = NULL;

void __do_global_dtors (void);
void __do_global_ctors (void);
void __main (void);

void
__do_global_dtors (void)
{
  static func_ptr *p = __DTOR_LIST__ + 1;

  while (*p)
    {
      (*(p)) ();
      p++;
    }
  if (free_Msvcrt && hMsvcrt)
    {
      free_Msvcrt = 0;
      FreeLibrary (hMsvcrt);
      hMsvcrt = NULL;
    }
}

void
__do_global_ctors (void)
{
  unsigned long nptrs = (unsigned long) (ptrdiff_t) __CTOR_LIST__[0];
  unsigned long i;

  if (!hMsvcrt) {
    hMsvcrt = GetModuleHandleA ("msvcr80.dll");
    if (!hMsvcrt)
      hMsvcrt = GetModuleHandleA ("msvcr70.dll");
    if (!hMsvcrt)
      hMsvcrt = GetModuleHandleA ("msvcrt.dll");
    if (!hMsvcrt) {
      hMsvcrt = LoadLibraryA ("msvcrt.dll");
      free_Msvcrt = 1;
    }
    fctMsvcrtLongJmp = (flongjmp *) GetProcAddress( hMsvcrt, "longjmp");
  }

  if (nptrs == (unsigned long) -1)
    {
      for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++);
    }

  for (i = nptrs; i >= 1; i--)
    {
      __CTOR_LIST__[i] ();
    }

  atexit (__do_global_dtors);
}

static int initialized = 0;

void
__main (void)
{
  if (!initialized)
    {
      initialized = 1;
      __do_global_ctors ();
    }
}

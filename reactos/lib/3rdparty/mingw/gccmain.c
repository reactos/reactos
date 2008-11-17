/*
 * gccmain.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * A separate version of __main, __do_global_ctors and __do_global_dtors for
 * Mingw32 for use with Cygwin32 b19. Hopefully this object file will only
 * be linked if the libgcc.a doesn't include __main, __do_global_dtors and
 * __do_global_ctors.
 *
 */
#include <windows.h>
#include <stdlib.h>
#include <setjmp.h>

typedef void (*func_ptr) (void);
extern func_ptr __CTOR_LIST__[];
extern func_ptr __DTOR_LIST__[];

static HMODULE hMsvcrt = NULL;

typedef void __cdecl flongjmp(jmp_buf _Buf,int _Value);

flongjmp *fctMsvcrtLongJmp = NULL;

void
__do_global_dtors (void)
{
  static func_ptr *p = __DTOR_LIST__ + 1;

  while (*p)
    {
      (*(p)) ();
      p++;
    }
  if (hMsvcrt)
    {
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
    hMsvcrt = LoadLibrary ("msvcrt.dll");
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

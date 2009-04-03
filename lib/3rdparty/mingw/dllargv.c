/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifdef CRTDLL
#undef CRTDLL
#endif

#include <internal.h>

#ifdef WPRFLAG
int __CRTDECL
_wsetargv (void)
#else
int __CRTDECL
_setargv (void)
#endif
{
  return 0;
}

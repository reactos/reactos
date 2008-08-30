#ifdef CRTDLL
#undef CRTDLL
#endif

#include "internal.h"

extern int _dowildcard;

#ifdef WPRFLAG
int __CRTDECL
__wsetargv (void)
#else
int __CRTDECL
__setargv (void)
#endif
{
  _dowildcard = 1;
  return 0;
}

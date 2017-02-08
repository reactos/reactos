/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
long CDECL atol(const char *str)
{
    return (long)_atoi64(str);
}

/*
 * @unimplemented
 */
int CDECL _atoldbl(_LDOUBLE *value, char *str)
{
  /* FIXME needs error checking for huge/small values */
#if 0
  long double ld;
  ld = strtold(str,0);
  memcpy(value, &ld, 10);
#endif
  return 0;
}

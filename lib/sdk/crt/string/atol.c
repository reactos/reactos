/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>
#include <tchar.h>

/*
 * @implemented
 */
long
_ttol(const _TCHAR *str)
{
  return _tcstol(str, 0, 10);
}

int _atoldbl(long double *value, const char *str)
{
    /* FIXME needs error checking for huge/small values */
   //*value = strtold(str,0);
   return 0;
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>
#include <tchar.h>

/*
 * @implemented
 */
long
_ttol(const _TCHAR *str)
{
  return (long)_ttoi64(str);
}

int _atoldbl(_LDOUBLE *value, char *str)
{
    /* FIXME needs error checking for huge/small values */
   //*value = strtold(str,0);
   return -1;
}

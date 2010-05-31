/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>
#include <stdlib.h>

/*
 * @implemented
 */
double CDECL
atof(const char *str)
{
  return _strtod_l(str, NULL,NULL);
}

double CDECL _atof_l( const char *str, _locale_t locale)
{
    return _strtod_l(str, NULL, locale);
}

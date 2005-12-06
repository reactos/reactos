/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
double
atof(const char *ascii)
{
  return strtod(ascii, 0);
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

/*
 * @implemented
 */
#ifndef _MSC_VER
int
abs(int j)
{
  return j<0 ? -j : j;
}
#endif

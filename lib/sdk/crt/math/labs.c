/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
#ifndef _MSC_VER
long
labs(long j)
{
  return j<0 ? -j : j;
}
#endif

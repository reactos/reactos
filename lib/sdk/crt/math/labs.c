/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#ifndef _MSC_VER
/*
 * @implemented
 */
long
labs(long j)
{
  return j<0 ? -j : j;
}
#endif

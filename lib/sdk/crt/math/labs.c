/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#ifdef _MSC_VER
#pragma warning(disable: 4164)
#pragma function(labs)
#endif

/*
 * @implemented
 */
long
labs(long j)
{
  return j<0 ? -j : j;
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/time.h>

/*
 * @implemented
 */
double
difftime(time_t time1, time_t time0)
{
  return time1-time0;
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
double
difftime(time_t time1, time_t time2)
{
  return (double)(time1 - time2);
}

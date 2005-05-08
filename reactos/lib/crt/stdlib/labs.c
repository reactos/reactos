/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

/*
 * @implemented
 */
long
labs(long j)
{
  return j<0 ? -j : j;
}

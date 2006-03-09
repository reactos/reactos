/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <stdlib.h>
#include <internal/tls.h>

/*
 * @implemented
 */
int
rand(void)
{
  PTHREADDATA ThreadData = GetThreadData();

  ThreadData->tnext = ThreadData->tnext * 0x5deece66dLL + 11;
  return (int)((ThreadData->tnext >> 16) & RAND_MAX);
}

/*
 * @implemented
 */
void
srand(unsigned int seed)
{
  PTHREADDATA ThreadData = GetThreadData();

  ThreadData->tnext = (ULONGLONG)seed;
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/tls.h>

int
rand(void)
{
  PTHREADDATA ThreadData = GetThreadData();

  ThreadData->tnext = ThreadData->tnext * 0x5deece66dLL + 11;
  return (int)((ThreadData->tnext >> 16) & RAND_MAX);
}

void
srand(unsigned int seed)
{
  PTHREADDATA ThreadData = GetThreadData();

  ThreadData->tnext = (unsigned long long)seed;
}

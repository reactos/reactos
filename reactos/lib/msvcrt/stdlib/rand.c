/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/tls.h>

/*
 * @implemented
 */
int
rand(void)
{
  PTHREADDATA ThreadData = GetThreadData();

#ifdef HAVE_LONGLONG
  ThreadData->tnext = ThreadData->tnext * 0x5deece66dLL + 11;
#else
  ThreadData->tnext = ThreadData->tnext * 0x5deece66dL + 11;
#endif
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

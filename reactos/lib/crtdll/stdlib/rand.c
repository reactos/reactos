/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>

static unsigned long long next = 0;

int
rand(void)
{
  next = next * 0x5deece66dLL + 11;
  return (int)((next >> 16) & RAND_MAX);
}

void
srand(unsigned seed)
{
  next = seed;
}

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

void *
bsearch(const void *key, const void *base0, size_t nelem,
	size_t size, int (*cmp)(const void *ck, const void *ce))
{
  char *base = (char *)base0;
  int lim, cmpval;
  void *p;

  for (lim = nelem; lim != 0; lim >>= 1)
  {
    p = base + (lim >> 1) * size;
    cmpval = (*cmp)(key, p);
    if (cmpval == 0)
      return p;
    if (cmpval > 0)
    {				/* key > p: move right */
      base = (char *)p + size;
      lim--;
    } /* else move left */
  }
  return 0;
}

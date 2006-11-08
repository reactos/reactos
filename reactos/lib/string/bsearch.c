#include <string.h>

/*
 * @implemented
 */
void *
bsearch(const void *key, const void *base0, size_t nelem,
	size_t size, int (*cmp)(const void *ck, const void *ce))
{
  const char *base = base0;
  int lim, cmpval;
  const void *p;

  for (lim = nelem; lim != 0; lim >>= 1)
  {
    p = base + (lim >> 1) * size;
    cmpval = (*cmp)(key, p);
    if (cmpval == 0)
      return (void*)((size_t)p);
    if (cmpval > 0)
    {				/* key > p: move right */
      base = (const char *)p + size;
      lim--;
    } /* else move left */
  }
  return 0;
}

#include <msvcrt/search.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

void *_lsearch(const void *key, void *base, size_t *nelp, size_t width,
         int (*compar)(const void *, const void *))
{
  void *ret_find = _lfind(key,base,nelp,width,compar);

  if (ret_find != NULL)
    return ret_find;

#ifdef __GNUC__
  memcpy(base + (*nelp*width), key, width);
#else
  memcpy((int*)base + (*nelp*width), key, width);
#endif
  (*nelp)++;
  return base;
}


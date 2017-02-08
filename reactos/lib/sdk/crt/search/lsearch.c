#include <stdlib.h>
#include <string.h>
#include <search.h>

/*
 * @implemented
 */
void * __cdecl _lsearch(const void *key, void *base, unsigned int *nelp, unsigned int width,
         int (__cdecl *compar)(const void *, const void *))
{
  void *ret_find = _lfind(key,base,nelp,width,compar);

  if (ret_find != NULL)
    return ret_find;

  memcpy((void*)((int*)base + (*nelp*width)), key, width);

  (*nelp)++;
  return base;
}



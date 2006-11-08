#include <string.h>
/*
 * @implemented
 */
void *_lfind(const void *key, const void *base, size_t *nelp,
         size_t width, int (*compar)(const void *, const void *))
{
  const char* char_base = (const char *)base;
  size_t i;

  for (i = 0; i < *nelp; i++)
    {
      if (compar(key, char_base) == 0)
	return (void *)((size_t)char_base);

      char_base += width;
    }

  return NULL;
}

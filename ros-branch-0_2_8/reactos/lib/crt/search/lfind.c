#include <search.h>
#include <stdlib.h>


/*
 * @implemented
 */
void *_lfind(const void *key, const void *base, size_t *nelp,
         size_t width, int (*compar)(const void *, const void *))
{
	char* char_base = (char*)base;
	unsigned int i;

    for (i = 0; i < *nelp; i++) {
		if (compar(key, char_base) == 0)
			return char_base;
		char_base += width;
	}
	return NULL;
}


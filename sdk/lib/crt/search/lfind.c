#include <stdlib.h>
#include <search.h>

/*
 * @implemented
 */
void * __cdecl _lfind(const void *key, const void *base, unsigned int *nelp,
         unsigned int width, int (__cdecl *compar)(const void *, const void *))
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



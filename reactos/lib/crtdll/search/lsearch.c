#include <crtdll/search.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>

void *_lsearch(const void *key, void *base, size_t *nelp, size_t width,
         int (*compar)(const void *, const void *))
{
	void *ret_find = _lfind(key,base,nelp,width,compar);
	if ( ret_find != NULL )
		return ret_find;

	memcpy( base + (*nelp*width), key, width );
        (*nelp)++;
        return base ;
}
#include <wstring.h>


wchar_t *wcsdup(const wchar_t *ptr)
{
	wchar_t *dup;
	dup = malloc((wcslen(ptr) + 1)*sizeof(wchar_t));
	if( dup == NULL )
		return NULL;
	wcscpy(dup,ptr);
	return dup;
}




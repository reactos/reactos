#include <msvcrt/ctype.h>
#include <msvcrt/string.h>

wchar_t *_wcsupr(wchar_t *x)
{
	wchar_t  *y = x;

	while (*y) {
		*y = towupper(*y);
		y++;
	}
	return x;
}

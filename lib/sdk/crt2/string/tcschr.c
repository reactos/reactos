/* $Id: tcschr.h 30283 2007-11-08 21:06:20Z fireball $
 */

#ifdef _UNICODE
#error
#endif

#include <tchar.h>

#ifdef _UNICODE
#error
#endif


_TCHAR * _CDECL _tcschr(const _TCHAR * s, _XINT c)
{
	_TCHAR cc = c;

	while(*s)
	{
		if(*s == cc)
			return (_TCHAR *)s;
		s++;
	}

	if(!cc)
		return (_TCHAR *)s;

	return 0;
}

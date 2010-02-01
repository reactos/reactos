/* wcscmp.c
(c) 2010 Jose M. Catena Gomez
*/

_NOINTRINSIC(wcscmp)

int _CDECL wcscmp(const wchar_t *str1, const wchar_t *str2)
{
	wchar_t r = 0;

	while(*str1)
	{
		r = *str1++ - *str2++;
		if(r)
			break;
	}
	return((int)r);
}

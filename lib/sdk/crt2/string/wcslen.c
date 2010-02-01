/* wcslen.c
(c) 2010 Jose Catena
*/

_NOINTRINSIC(wcslen)

size_t _CDECL wcslen(const wchar_t *str)
{
	size_t i;

	for(i=0; str[i]; i++);
	return(i);
}

/* wcsncmp.c
(c) 2010 Jose Catena
*/

int _CDECL wcsncmp(const wchar_t *str1, const wchar_t *str2, size_t cnt)
{
	i16 r = 0;

	while(*str1 && cnt--)
	{
		r = *str1++ - *str2++;
		if(r)
			break;
	}
	return((int)r);
}

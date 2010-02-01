/* wcsncat.c
(c) 2010 Jose Catena
*/

wchar_t * _CDECL wcsncat(wchar_t *dst, const wchar_t *src, size_t cnt)
{
	wchar_t *dst0 = dst;

	while(*dst)
		dst++;
	while(*src && cnt--)
	{
		*dst++ = *src++;
	}
	*dst = 0;
	return(dst0);
}

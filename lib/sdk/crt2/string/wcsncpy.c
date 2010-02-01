/* wcsncpy.c
(c) 2010 Jose Catena
*/

wchar_t * _CDECL wcsncpy(wchar_t * dst, const wchar_t * src, size_t cnt)
{
	size_t i;

	for(i=0; cnt && (dst[i] = src[i]); i++, cnt--);
	while(cnt--)
		dst[i++] = 0;
	return(dst);
}

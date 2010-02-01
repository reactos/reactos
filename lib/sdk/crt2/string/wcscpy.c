/* wcscpy.c
(c) 2010 Jose Catena
*/

_NOINTRINSIC(wcscpy)

wchar_t * _CDECL wcscpy(wchar_t *dst, const wchar_t *src)
{
	size_t i;

	for(i=0;;i++)
	{
		dst[i] = src[i];
		if(!src[i])
			break;
	}
	return(dst);
}

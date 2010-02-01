/* strncpy.c
(c) 2010 Jose Catena
*/

char * _CDECL strncpy(char * dst, const char * src, size_t cnt)
{
	size_t i;

	for(i=0; cnt && (dst[i] = src[i]); i++, cnt--);
	while(cnt--)
		dst[i++] = 0;
	return(dst);
}

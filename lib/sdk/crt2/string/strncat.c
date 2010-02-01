/* strncat.c
(c) 2010 Jose Catena
*/

char * _CDECL strncat(char *dst, const char *src, size_t cnt)
{
	char *dst0 = dst;

	while(*dst)
		dst++;
	while(*src && cnt--)
	{
		*dst++ = *src++;
	}
	*dst = 0;
	return(dst0);
}

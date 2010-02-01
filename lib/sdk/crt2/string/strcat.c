/* strcat.c
(c) 2010 Jose Catena
*/

_NOINTRINSIC(strcat)

char * _CDECL strcat(char *dst, const char *src)
{
	char *dst0 = dst;

	while(*dst)
		dst++;
	while(*src)
	{
		*dst++ = *src++;
	}
	*dst = 0;
	return(dst0);
}

/* wcscpy.c
(c) 2010 Jose Catena
*/

_NOINTRINSIC(strcpy)

char * _CDECL strcpy(char *dst, const char *src)
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

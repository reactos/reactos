/* wcscmp.c
(c) 2010 Jose M. Catena Gomez
*/

_NOINTRINSIC(strcmp)

int _CDECL strcmp(const char *str1, const char *str2)
{
	char r = 0;

	while(*str1)
	{
		r = *str1++ - *str2++;
		if(r)
			break;
	}
	return((int)r);
}

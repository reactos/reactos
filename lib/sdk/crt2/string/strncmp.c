/* strncmp.c
(c) 2010 Jose Catena
*/

int _CDECL strncmp(const char *str1, const char *str2, size_t cnt)
{
	char r = 0;

	while(*str1 && cnt--)
	{
		r = *str1++ - *str2++;
		if(r)
			break;
	}
	return((int)r);
}

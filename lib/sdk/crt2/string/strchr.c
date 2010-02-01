/* $Id: strchr.c 30283 2007-11-08 21:06:20Z fireball $
 */

char * _CDECL strchr(const char *s, int c)
{
	int cc = c;

	while(*s)
	{
		if(*s == c)
			return (char *)s;
		s++;
	}

	if(!c)
		return (char *)s;

	return 0;
}

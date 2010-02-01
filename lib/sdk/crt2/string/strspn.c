/* strspn.c
(c) 2010 Jose Catena
*/

size_t _CDECL strspn(const char *str, const char *charset)
{
	size_t i, j;

	for(i=0; str[i]; i++)
	{
		for(j=0; charset[j]; j++)
		{
			if(str[i] == charset[j])
				break;
		}
		if(!charset[j])
			break;
	}
	return(i);
}


/* wcscspn.c
(c) 2010 Jose Catena
*/

size_t _CDECL wcscspn(const wchar_t *str, const wchar_t *charset)
{
	size_t i, j;

	for(i = 0; str[i]; i++)
	{
		for(j=0; charset[j]; j++)
		{
			if(str[i] == charset[j])
				return(i);
		}
	}
	return(i);
}

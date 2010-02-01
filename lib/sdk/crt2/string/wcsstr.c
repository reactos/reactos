/*
wcsstr.c
(c) Jose Catena
*/
wchar_t * _CDECL wcsstr(const wchar_t *str1, const wchar_t *str2)
{
	size_t i;
	wchar_t c1, c2;

	for(;;)
	{
		for(i=0;;i++)
		{
			c2 = str2[i];
			if(!c2)
				break;
			c1 = str1[i];
			if(c1 != c2)
				break;
		}
		if(!c2)
			return((wchar_t *)str1);
		if(!c1)
			break;
		str1++;
	}
	return(0);
}

/*
strstr.c
(c) Jose Catena
*/
char * _CDECL strstr(const char *str1, const char *str2)
{
	size_t i;
	char c1, c2;

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
			return((char *)str1);
		if(!c1)
			break;
		str1++;
	}
	return(0);
}

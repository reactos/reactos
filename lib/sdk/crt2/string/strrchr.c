char * _CDECL strrchr (const char *str, int c)
{
	char *str1 = (char *)str;

	while(*str1++);
	while(--str1 != str && *str1 != (char)c);
	if(*str1 == (char)c)
		return (char *)str1;
	return(0);
}

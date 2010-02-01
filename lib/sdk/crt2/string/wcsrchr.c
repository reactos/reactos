wchar_t * _CDECL wcsrchr (const wchar_t *str, wchar_t c)
{
	wchar_t *str1 = (wchar_t *)str;

	while(*str1++);
	while(--str1 != str && *str1 != c);
	if(*str1 == c)
		return(str1);
	return(0);
}

wchar_t * _CDECL wcschr(const wchar_t * str, wchar_t c)
{
	while(*str && *str != c)
		str++;
	if(*str == c)
		return (wchar_t *)str;
	return 0;
}

wchar_t* _CDECL _wcsrev( wchar_t* str )
{
	wchar_t *r = str;
	wchar_t *end = str;
	while(*end)
		end++;
	while (end > str)
	{
		wchar_t t = *end;
		*end--  = *str;
		*str++  = t;
	}
	return r;
}

wchar_t* wcsrchr(const wchar_t* str, wchar_t ch)
{
  
	wchar_t *sp=(wchar_t *)0;
	while (*str != 0)
	{
		if (*str == ch) 
			sp = (wchar_t *)str;
		str++;
	}
	if (ch == 0)
		sp = (wchar_t *)str;
	return sp;
}

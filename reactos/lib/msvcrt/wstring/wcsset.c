#include <msvcrt/string.h>

wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill)
{
	wchar_t *t = wsToFill;
	int i = 0;
	while( *wsToFill != 0 && i < sizeMaxFill)
	{
		*wsToFill = wcFill;
		wsToFill++;
		i++;
		
	}
	return t;
}

wchar_t* _wcsset (wchar_t* wsToFill, wchar_t wcFill)
{
	wchar_t *t = wsToFill;
	while( *wsToFill != 0 )
	{
		*wsToFill = wcFill;
		wsToFill++;
		
	}
	return t;
}

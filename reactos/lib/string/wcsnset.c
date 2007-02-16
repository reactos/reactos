#include <string.h>

/*
 * @implemented
 */
wchar_t *_wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill)
{
	wchar_t *t = wsToFill;
	int i = 0;
	while( *wsToFill != 0 && i < (int) sizeMaxFill)
	{
		*wsToFill = wcFill;
		wsToFill++;
		i++;
	}
	return t;
}

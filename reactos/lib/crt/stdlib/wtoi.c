#include <msvcrt/stdlib.h>

/*
 * @implemented
 */
int _wtoi( const wchar_t *str )
{
	return (int)wcstol(str, 0, 10);
}

/*
 * @implemented
 */
long _wtol( const wchar_t *str )
{
	return (int)wcstol(str, 0, 10);
}

#include <crtdll/wchar.h>

int _wtoi( const wchar_t *str )
{
	return (int)wcstol(str, 0, 10);
}
  
long _wtol( const wchar_t *str )
{
	return (int)wcstol(str, 0, 10);
}
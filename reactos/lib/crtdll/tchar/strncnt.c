#include <crtdll/tchar.h>
#include <crtdll/sys/types.h>
#include <crtdll/string.h>

size_t _strncnt( const char *str, size_t max) 
{ 
	return strnlen(str,max); 
}

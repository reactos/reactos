#include <msvcrt/tchar.h>
#include <msvcrt/sys/types.h>
#include <msvcrt/string.h>

size_t _strncnt( const char *str, size_t max) 
{ 
	return strnlen(str,max); 
}

#include <msvcrt/tchar.h>
#include <msvcrt/stdlib.h>

char * _strninc(const char *str, size_t inc)
{ 
	return (char *)(str + inc); 
}

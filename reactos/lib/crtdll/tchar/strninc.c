#include <crtdll/tchar.h>
#include <crtdll/stdlib.h>

char * _strninc(const char *str, size_t inc)
{ 
	return (char *)(str + inc); 
}

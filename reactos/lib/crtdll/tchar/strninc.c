#include <crtdll/tchar.h>

char * _strninc(const char *str, size_t inc)
{ 
	return (char *)(str + inc); 
}

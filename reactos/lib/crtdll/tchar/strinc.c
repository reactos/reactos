#include <msvcrt/tchar.h>

char * _strinc(const char *str) 
{ 
	return (char *)(++str); 
}

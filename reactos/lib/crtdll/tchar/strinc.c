#include <msvcrt/string.h>

char * _strinc(const char *str) 
{ 
	return (char *)(++str); 
}

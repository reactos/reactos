#include <msvcrt/string.h>

/*
 * @implemented
 */
char * _strinc(const char *str) 
{ 
	return (char *)(++str); 
}

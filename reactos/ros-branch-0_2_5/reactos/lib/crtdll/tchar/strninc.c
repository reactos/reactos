#include <msvcrt/string.h>
#include <msvcrt/stdlib.h>

/*
 * @implemented
 */
char * _strninc(const char *str, size_t inc)
{ 
	return (char *)(str + inc); 
}

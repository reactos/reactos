#include <msvcrt/sys/types.h>
#include <msvcrt/string.h>

/*
 * @implemented
 */
size_t _strncnt( const char *str, size_t max) 
{ 
	return strnlen(str,max); 
}

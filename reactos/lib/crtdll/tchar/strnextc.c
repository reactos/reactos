#include <msvcrt/string.h>

/*
 * @implemented
 */
int _strnextc(const char *str) 
{ 
	return *(++str); 
}

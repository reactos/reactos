#include <msvcrt/string.h>

int _strnextc(const char *str) 
{ 
	return *(++str); 
}

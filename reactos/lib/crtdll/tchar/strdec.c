#include <msvcrt/tchar.h>

char * _strdec(const char *str1, const char *str2) 
{ 
	return (char *) (( str1 >= str2 ) ? ( str1 ) : --str2); 
}


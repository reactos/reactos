#include <windows.h>
#include <stdlib.h>

void *malloc(size_t size);



char *getenv(const char *name) 
{
	char *buffer;
	buffer = (char *)malloc(MAX_PATH);
	buffer[0] = 0;
	if ( GetEnvironmentVariableA(name,buffer,MAX_PATH) == 0 )
		return NULL;
 	return buffer;
}

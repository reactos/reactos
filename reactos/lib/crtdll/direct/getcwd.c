#include <windows.h>
#include <crtdll/direct.h>
#include <crtdll/stdlib.h>



char *_getcwd( char *buffer, int maxlen )
{
	char *cwd;
	int len;
	if ( buffer == NULL ) {
		cwd = malloc(MAX_PATH);
		len = MAX_PATH;
	}
	else {
		cwd = buffer;
		len = maxlen;
	}
	

	if ( GetCurrentDirectory(len,cwd) == 0 )
		return NULL;


	return cwd;
}


#include <msvcrti.h>


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

	if ( GetCurrentDirectoryA(len,cwd) == 0 )
		return NULL;

	return cwd;
}

wchar_t *_wgetcwd( wchar_t *buffer, int maxlen )
{
	wchar_t *cwd;
	int len;
	if ( buffer == NULL ) {
		cwd = malloc(MAX_PATH * sizeof(wchar_t));
		len = MAX_PATH;
	}
	else {
		cwd = buffer;
		len = maxlen;
	}

	if ( GetCurrentDirectoryW(len,cwd) == 0 )
		return NULL;

	return cwd;
}

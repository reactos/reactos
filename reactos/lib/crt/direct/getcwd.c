#include "precomp.h"
#include <direct.h>
#include <stdlib.h>
#include <errno.h>
#include <internal/file.h>


/*
 * @implemented
 */
char *_getcwd(char* buffer, int maxlen)
{
    char *cwd;
    int len;

    if (buffer == NULL) {
        if ( (cwd = malloc(MAX_PATH)) == NULL ) {
        	__set_errno(ENOMEM);
        	return NULL;
        }
        len = MAX_PATH;
    } else {
        cwd = buffer;
        len = maxlen;
    }
    if (GetCurrentDirectoryA(len, cwd) == 0) {
    	_dosmaperr(GetLastError());
        return NULL;
    }
    return cwd;
}

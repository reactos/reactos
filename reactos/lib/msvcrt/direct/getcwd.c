#include "precomp.h"
#include <msvcrt/direct.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


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

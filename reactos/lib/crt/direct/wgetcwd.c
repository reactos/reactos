#include "precomp.h"
#include <direct.h>
#include <stdlib.h>
#include <errno.h>
#include <internal/file.h>


/*
 * @implemented
 */
wchar_t* _wgetcwd(wchar_t* buffer, int maxlen)
{
    wchar_t *cwd;
    int len;
    if (buffer == NULL) {
        if ( (cwd = malloc(MAX_PATH * sizeof(wchar_t))) == NULL ) {
        	__set_errno(ENOMEM);
        	return NULL;
        }
        len = MAX_PATH;
    } else {
        cwd = buffer;
        len = maxlen;
    }
    if (GetCurrentDirectoryW(len, cwd) == 0)
        return NULL;
    return cwd;
}

#include "precomp.h"
#include <msvcrt/direct.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
int _mkdir(const char* _path)
{
    if (!CreateDirectoryA(_path, NULL)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}

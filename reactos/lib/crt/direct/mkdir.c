#include "precomp.h"
#include <direct.h>
#include <internal/file.h>


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

#include "precomp.h"
#include <direct.h>
#include <internal/file.h>


/*
 * @implemented
 */
int _rmdir(const char* _path)
{
    if (!RemoveDirectoryA(_path)) {
    	_dosmaperr(GetLastError());
        return -1;
    }
    return 0;
}

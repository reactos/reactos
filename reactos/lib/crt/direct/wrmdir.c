#include "precomp.h"
#include <direct.h>
#include <internal/file.h>


/*
 * @implemented
 */
int _wrmdir(const wchar_t* _path)
{
    if (!RemoveDirectoryW(_path)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}

#include "precomp.h"
#include <msvcrt/direct.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
int _wmkdir(const wchar_t* _path)
{
    if (!CreateDirectoryW(_path, NULL)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}

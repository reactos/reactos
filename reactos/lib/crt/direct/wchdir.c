#include "precomp.h"
#include <ctype.h>
#include <direct.h>
#include <internal/file.h>

/*
 * @implemented
 */
int _wchdir (const wchar_t *_path)
{
    if (!SetCurrentDirectoryW((wchar_t *)_path)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}

#include "precomp.h"
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>
#include <msvcrt/internal/file.h>

/*
 * @implemented
 */
int _chdir(const char* _path)
{
    if (!SetCurrentDirectoryA((char*)_path)) {
    	_dosmaperr(GetLastError());
		return -1;
	}
    return 0;
}

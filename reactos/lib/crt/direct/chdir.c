#include "precomp.h"
#include <ctype.h>
#include <direct.h>
#include <internal/file.h>

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

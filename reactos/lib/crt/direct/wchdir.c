#include "precomp.h"
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>
#include <msvcrt/internal/file.h>
#include <msvcrt/internal/file.h>

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

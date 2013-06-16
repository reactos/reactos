#include <precomp.h>
#include <direct.h>
#include <tchar.h>

/*
 * @implemented
 */
int _trmdir(const _TCHAR* _path)
{
    if (!RemoveDirectory(_path)) {
    	_dosmaperr(GetLastError());
        return -1;
    }
    return 0;
}

#include <precomp.h>
#include <direct.h>
#include <tchar.h>

/*
 * @implemented
 */
int _tmkdir(const _TCHAR* _path)
{
    if (!CreateDirectory(_path, NULL)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}

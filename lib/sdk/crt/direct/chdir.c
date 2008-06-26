#include <precomp.h>
#include <ctype.h>
#include <direct.h>
#include <tchar.h>

/*
 * @implemented
 */
int _tchdir(const _TCHAR* _path)
{
    if (!SetCurrentDirectory(_path)) {
      _dosmaperr(_path?GetLastError():0);
		return -1;
	}
    return 0;
}

#include <windows.h>
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>


int _wchdir (const wchar_t *_path)
{
    if (_path[1] == L':')
        _chdrive(towlower(_path[0] - L'a')+1);
    if (!SetCurrentDirectoryW((wchar_t *)_path))
        return -1;
    return 0;
}

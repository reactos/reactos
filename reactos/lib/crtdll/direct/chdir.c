#include <windows.h>
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>


int _chdir(const char* _path)
{
    if (_path[1] == ':')
        _chdrive(tolower(_path[0] - 'a')+1);
    if (!SetCurrentDirectoryA((char*)_path))
        return -1;
    return 0;
}

#include <windows.h>
#include <msvcrt/direct.h>


/*
 * @implemented
 */
int _wmkdir(const wchar_t* _path)
{
    if (!CreateDirectoryW(_path, NULL))
        return -1;
    return 0;
}

#include <windows.h>
#include <msvcrt/direct.h>

/*
 * @implemented
 */
int _wrmdir(const wchar_t* _path)
{
    if (!RemoveDirectoryW(_path))
        return -1;
    return 0;
}

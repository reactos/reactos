#include <windows.h>
#include <msvcrt/direct.h>


/*
 * @implemented
 */
int _rmdir(const char* _path)
{
    if (!RemoveDirectoryA(_path))
        return -1;
    return 0;
}

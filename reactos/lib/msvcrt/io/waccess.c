#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


/*
 * @implemented
 */
int _waccess(const wchar_t *_path, int _amode)
{
    DWORD Attributes = GetFileAttributesW(_path);

    if (Attributes == -1) {
        __set_errno(ENOENT);
        return -1;
    }
    if ((_amode & W_OK) == W_OK) {
        if ((Attributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY) {
            __set_errno(EACCES);
            return -1;
        }
    }
    if ((_amode & D_OK) == D_OK) {
        if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
            __set_errno(EACCES);
            return -1;
        }
    }
    return 0;
}

#include <windows.h>
#include <msvcrt/direct.h>

/*
 * @implemented
 */
char* _getdcwd(int nDrive, char* caBuffer, int nBufLen)
{
    int i =0;
    int dr = _getdrive();

    if (nDrive < 1 || nDrive > 26)
        return NULL;
    if (dr != nDrive)
        _chdrive(nDrive);
    i = GetCurrentDirectoryA(nBufLen, caBuffer);
    if (i  == nBufLen)
        return NULL;
    if (dr != nDrive)
        _chdrive(dr);
    return caBuffer;
}

#include <windows.h>
#include <msvcrt/direct.h>


/*
 * @implemented
 */
wchar_t* _wgetdcwd(int nDrive, wchar_t* caBuffer, int nBufLen)
{
    int i =0;
    int dr = _getdrive();

    if (nDrive < 1 || nDrive > 26)
        return NULL;

    if (dr != nDrive)
        _chdrive(nDrive);

    i = GetCurrentDirectoryW(nBufLen, caBuffer);
    if (i  == nBufLen)
        return NULL;

    if (dr != nDrive)
        _chdrive(dr);

    return caBuffer;
}

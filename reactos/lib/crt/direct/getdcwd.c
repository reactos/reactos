#include "precomp.h"
#include <direct.h>
#include <internal/file.h>

/*
 * @implemented
 */
char* _getdcwd(int nDrive, char* caBuffer, int nBufLen)
{
    int i =0;
    int dr = _getdrive();

    if (nDrive < 1 || nDrive > 26)
        return NULL;
    if (dr != nDrive) {
        if ( _chdrive(nDrive) != 0 )
        	return NULL;
	}
    i = GetCurrentDirectoryA(nBufLen, caBuffer);
    if (i == nBufLen)
        return NULL;
    if (dr != nDrive) {
        if ( _chdrive(dr) != 0 )
        	return NULL;
	}
    return caBuffer;
}

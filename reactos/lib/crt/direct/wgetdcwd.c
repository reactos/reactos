#include "precomp.h"
#include <direct.h>
#include <internal/file.h>


/*
 * @implemented
 */
wchar_t* _wgetdcwd(int nDrive, wchar_t* caBuffer, int nBufLen)
{
    int i =0;
    int dr = _getdrive();

    if (nDrive < 1 || nDrive > 26)
        return NULL;

    if (dr != nDrive) {
        if ( _chdrive(nDrive) != 0 )
        	return NULL;
	}

    i = GetCurrentDirectoryW(nBufLen, caBuffer);
    if (i == nBufLen)
        return NULL;

    if (dr != nDrive) {
    	if ( _chdrive(dr) != 0 )
			return NULL;
	}

    return caBuffer;
}

#include "precomp.h"
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


int cur_drive = 0;


/*
 * @implemented
 */
int _chdrive(int drive)
{
    char d[3];

    if (!( drive >= 1 && drive <= 26)) {
    	__set_errno(EINVAL);
        return -1;
	}
    if (cur_drive != drive) {
        cur_drive = drive;
        d[0] = toupper(cur_drive + '@');
        d[1] = ':';
        d[2] = 0;
        if (!SetCurrentDirectoryA(d)) {
	    	_dosmaperr(GetLastError());
	    	return -1;
	    }	
    }
    return 0;
}

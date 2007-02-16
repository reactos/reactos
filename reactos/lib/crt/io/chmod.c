/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Created
 */

#include <precomp.h>
#include <sys/stat.h>
#include <tchar.h>

#define NDEBUG
#include <internal/debug.h>


#define mode_t int


/*
 * @implemented
 */
int _tchmod(const _TCHAR* filename, mode_t mode)
{
    DWORD FileAttributes = 0;
    BOOLEAN Set = FALSE;

    DPRINT(MK_STR(_tchmod)"('%"sT"', %x)\n", filename, mode);

    FileAttributes = GetFileAttributes(filename);
    if ( FileAttributes == (DWORD)-1 ) {
    	_dosmaperr(GetLastError());
        return -1;
    }

    if ( mode == 0 )
        return -1;

    if (mode & _S_IWRITE) {
	if (FileAttributes & FILE_ATTRIBUTE_READONLY) {
	    FileAttributes &= ~FILE_ATTRIBUTE_READONLY;
	    Set = TRUE;
	}
    } else {
	if (!(FileAttributes & FILE_ATTRIBUTE_READONLY)) {
	    FileAttributes |= FILE_ATTRIBUTE_READONLY;
	    Set = TRUE;
	}
    }
    if (Set && SetFileAttributes(filename, FileAttributes) == FALSE) {
        _dosmaperr(GetLastError());
	return -1;
    }
    return 0;
}

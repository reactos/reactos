#include <windows.h>
#include <msvcrt/io.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

#define mode_t int


/*
 * @implemented
 */
int _wchmod(const wchar_t* filename, mode_t mode)
{
    DWORD FileAttributes = 0;
    BOOLEAN Set = FALSE;

    DPRINT("_wchmod('%S', %x)\n", filename, mode);

    FileAttributes = GetFileAttributesW(filename);
    if ( FileAttributes == -1 ) {
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

    if (Set && SetFileAttributesW(filename, FileAttributes) == FALSE) {
	_dosmaperr(GetLastError());
        return -1;
    }

    return 0;
}

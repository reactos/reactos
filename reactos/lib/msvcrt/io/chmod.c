#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

#define mode_t int


/*
 * @implemented
 */
int _chmod(const char* filename, mode_t mode)
{
    DWORD FileAttributes = 0;
    BOOLEAN Set = FALSE;

    DPRINT("_chmod('%s', %x)\n", filename, mode);

    FileAttributes = GetFileAttributesA(filename);
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
    if (Set && SetFileAttributesA(filename, FileAttributes) == FALSE) {
        _dosmaperr(GetLastError());
	return -1;
    }
    return 0;
}

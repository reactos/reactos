#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


BOOL STDCALL SetFilePointerEx(
  HANDLE hFile,                    // handle to file
  LARGE_INTEGER liDistanceToMove,  // bytes to move pointer
  PLARGE_INTEGER lpNewFilePointer, // new file pointer
  DWORD dwMoveMethod               // starting point
);

__int64 _lseeki64(int _fildes, __int64 _offset, int _whence)
{
#if 0
    __int64 new_pos;
    LARGE_INTEGER offset = _offset;

//    if (invalid_filehnd(_fildes)) {
//        errno = EBADF;
//        return -1L;
//    }
    if (SetFilePointerEx((HANDLE)filehnd(_fildes), offset, &new_pos, _whence)) {
    } else {
        //errno = EINVAL;
        return -1L;
    }
    return new_pos;
#else
    ULONG lo_pos, hi_pos;
    //DWORD lo_pos;

    lo_pos = SetFilePointer((HANDLE)filehnd(_fildes), _offset, &hi_pos, _whence);
    return((((__int64)hi_pos) << 32) + lo_pos);
#endif
}
/*
long    _lseek   ( int handle,    long offset, int origin );
__int64 _lseeki64( int handle, __int64 offset, int origin );

BOOL SetFilePointerEx(
  HANDLE hFile,                    // handle to file
  LARGE_INTEGER liDistanceToMove,  // bytes to move pointer
  PLARGE_INTEGER lpNewFilePointer, // new file pointer
  DWORD dwMoveMethod               // starting point
);
DWORD SetFilePointer(
  HANDLE hFile,                // handle to file
  LONG lDistanceToMove,        // bytes to move pointer
  PLONG lpDistanceToMoveHigh,  // bytes to move pointer
  DWORD dwMoveMethod           // starting point
);
 */

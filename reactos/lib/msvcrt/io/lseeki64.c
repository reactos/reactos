#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


//#define SETFILEPOINTEREX_AVAILABLE

__int64 _lseeki64(int _fildes, __int64 _offset, int _whence)
{
#ifdef SETFILEPOINTEREX_AVAILABLE
    LARGE_INTEGER new_pos;
    LARGE_INTEGER offset;
    offset.QuadPart = _offset;

//    if (invalid_filehnd(_fildes)) {
//        errno = EBADF;
//        return -1L;
//    }
    if (SetFilePointerEx((HANDLE)filehnd(_fildes), offset, &new_pos, _whence)) {
    } else {
        //errno = EINVAL;
        return -1L;
    }
    return new_pos.QuadPart;
#else
    //ULONG lo_pos;
    //DWORD hi_pos = 0;  // must equal 0 or -1 if supplied, -1 for negative 32 seek value
    //lo_pos = SetFilePointer((HANDLE)filehnd(_fildes), _offset, &hi_pos, _whence);
    //return((((__int64)hi_pos) << 32) + lo_pos);

    LARGE_INTEGER offset;
    offset.QuadPart = _offset;

    offset.u.LowPart = SetFilePointer((HANDLE)filehnd(_fildes), 
                          offset.u.LowPart, &offset.u.HighPart, _whence);
    return ((((__int64)offset.u.HighPart) << 32) + offset.u.LowPart);

#endif /*SETFILEPOINTEREX_AVAILABLE*/
}

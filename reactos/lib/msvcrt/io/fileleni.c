#include <windows.h>
#include <msvcrt/io.h>


/*
 * @implemented
 */
__int64 _filelengthi64(int _fd)
{
    long lo_length, hi_length;

    lo_length = GetFileSize(_get_osfhandle(_fd), &hi_length);
    return((((__int64)hi_length) << 32) + lo_length);
}

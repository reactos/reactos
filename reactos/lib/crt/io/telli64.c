#include <msvcrt/errno.h>
#include <msvcrt/io.h>


/*
 * @implemented
 */
__int64 _telli64(int _file)
{
    return _lseeki64(_file, 0, SEEK_CUR);
}

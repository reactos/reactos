#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
long _lseek(int _fildes, long _offset, int _whence)
{
    return (SetFilePointer((HANDLE)filehnd(_fildes), _offset, NULL, _whence));
}

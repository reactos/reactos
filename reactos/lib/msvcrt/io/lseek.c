#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


long _lseek(int _fildes, long _offset, int _whence)
{
  return (SetFilePointer((HANDLE)filehnd(_fildes), _offset, NULL, _whence));
}

__int64 _lseeki64(int _fildes, __int64 _offset, int _whence)
{
  ULONG lo_pos, hi_pos;

  lo_pos = SetFilePointer((HANDLE)filehnd(_fildes), _offset, &hi_pos, _whence);
  return((((__int64)hi_pos) << 32) + lo_pos);
}

#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>

int _creat(const char *filename, int mode)
{
  return _open(filename,_O_CREAT|_O_TRUNC,mode);
}

int _wcreat(const wchar_t *filename, int mode)
{
  return _wopen(filename,_O_CREAT|_O_TRUNC,mode);
}

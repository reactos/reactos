#include <msvcrt/io.h>


int _sopen(char *path, int access, int shflag, int mode)
{
  return _open((path), (access)|(shflag), (mode));
}

int _wsopen(wchar_t *path, int access, int shflag, int mode)
{
  return _wopen((path), (access)|(shflag), (mode));
}

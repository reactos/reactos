#include <msvcrti.h>


int
vsopen(const char *path, int access, int shflag, va_list args)
{
  int mode;

  mode = va_arg(args, int);
  return _open(path, access | shflag, mode);
}

int
vwsopen(const wchar_t *path, int access, int shflag, va_list args)
{
  int mode;

  mode = va_arg(args, int);
  return _wopen(path, access | shflag, mode);
}

int _sopen(const char *path, int access, int shflag, ...)
{
  va_list args;
  int retval;

  va_start (args, shflag);
  retval = vsopen(path, access, shflag, args);
  va_end (args);

  return retval;
}

int _wsopen(const wchar_t *path, int access, int shflag, ...)
{
  va_list args;
  int retval;

  va_start (args, shflag);
  retval = vwsopen(path, access, shflag, args);
  va_end (args);

  return retval;
}

#include <crtdll/io.h>
#include <crtdll/sys/stat.h>


int _isatty( int fd )
{
  struct stat buf;

  if (_fstat (fd, &buf) < 0)
    return 0;
  if (S_ISCHR (buf.st_mode))
    return 1;
  return 0;
}

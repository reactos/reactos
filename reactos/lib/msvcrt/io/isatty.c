#include <msvcrti.h>


#define NDEBUG
#include <msvcrtdbg.h>

int _isatty( int fd )
{
  struct _stat buf;
  DPRINT("_isatty(fd %d)\n", fd);
  if (_fstat (fd, &buf) < 0)
    return 0;
  if (S_ISCHR (buf.st_mode))
    return 1;
  return 0;
}

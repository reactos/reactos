#include <msvcrti.h>


#define NDEBUG
#include <msvcrtdbg.h>

int _close(int _fd)
{
  DPRINT("_close(fd %d)\n", _fd);
  if (_fd == -1)
    return -1;
  if (CloseHandle((HANDLE)_get_osfhandle(_fd)) == FALSE)
    return -1;
  return __fileno_close(_fd);
}

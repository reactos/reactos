#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>


int _stat(const char *path, struct stat *buffer)
{
  int fd = _open(path,_O_RDONLY);
  int ret;

  ret = fstat(fd,buffer);
  _close(fd);

  return ret;
}

#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>


int _stat(const char *path, struct stat *buffer)
{
  int fd = _open(path,_O_RDONLY);
  int ret;

  if (fd < 0)
    return -1;

  ret = fstat(fd,buffer);
  _close(fd);

  return ret;
}

__int64 _stati64 (const char *path, struct _stati64 *buffer)
{
  int fd = _open(path,_O_RDONLY);
  int ret;

  ret = _fstati64(fd,buffer);
  _close(fd);

  return ret;
}

int _wstat (const wchar_t *path, struct stat *buffer)
{
  int fd = _wopen(path,_O_RDONLY);
  int ret;

  ret = fstat(fd,buffer);
  _close(fd);

  return ret;
}

__int64 _wstati64 (const wchar_t *path, struct _stati64 *buffer)
{
  int fd = _wopen(path,_O_RDONLY);
  int ret;

  ret = _fstati64(fd,buffer);
  _close(fd);

  return ret;
}

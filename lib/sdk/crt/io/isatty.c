#include <precomp.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
int _isatty(int fd)
{
  HANDLE hFile = fdinfo(fd)->hFile;
  if (hFile == INVALID_HANDLE_VALUE)
    return 0;
  return GetFileType(hFile) == FILE_TYPE_CHAR ? 1 : 0;
}

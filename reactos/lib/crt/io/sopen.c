#include <msvcrt/io.h>


/*
 * @implemented
 */
int _sopen(char *path, int access, int shflag, int mode)
{
  return _open((path), (access)|(shflag), (mode));
}

#include <io.h>


/*
 * @implemented
 */
int _sopen(const char *path, int access, int shflag, ... /*mode, permissin*/)
{
   //FIXME: vararg
  return _open((path), (access)|(shflag));//, (mode));
}

#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


int _dup(int handle)
{
  return __fileno_alloc(_get_osfhandle(handle), __fileno_getmode(handle));
}

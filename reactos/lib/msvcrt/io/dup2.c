#include <msvcrti.h>


int _dup2(int handle1, int handle2)
{
  return __fileno_dup2(handle1, handle2);
}

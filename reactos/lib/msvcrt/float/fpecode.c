#include <msvcrt/float.h>
#include <msvcrt/internal/tls.h>

int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}

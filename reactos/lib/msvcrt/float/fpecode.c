#include <msvcrt/float.h>
#include <msvcrt/internal/tls.h>

/*
 * @implemented
 */
int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}

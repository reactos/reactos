#ifdef __USE_W32API
#undef __USE_W32API
#endif

#include <msvcrt/float.h>
#include <msvcrt/internal/tls.h>

/*
 * @implemented
 */
int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}

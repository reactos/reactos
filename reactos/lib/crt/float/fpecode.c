#ifdef __USE_W32API
#undef __USE_W32API
#endif

#include <float.h>
#include <internal/tls.h>

/*
 * @implemented
 */
int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}

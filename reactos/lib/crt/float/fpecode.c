#include "precomp.h"
#include <float.h>
#include <internal/tls.h>

/*
 * @implemented
 */
int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}

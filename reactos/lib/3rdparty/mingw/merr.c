#include "internal.h"
#include <math.h>

int __defaultmatherr = 1;

int __CRTDECL
_matherr (struct _exception *pexcept)
{
  /* Make compiler happy.  */
  pexcept = pexcept;
  return 0;
}

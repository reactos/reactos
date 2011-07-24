#include <math.h>
#include <float.h>
#include "ieee754/ieee754.h"

typedef int fpclass_t;
fpclass_t _fpclass(double __d);
int *_errno(void);

/*
 * @unimplemented
 */
double _j1(double num)
{
  /* FIXME: errno handling */
  return __ieee754_j1(num);
}

/*
 * @implemented
 */
double _y1(double num)
{
  double retval;
  if (!_finite(num)) *_errno() = EDOM;
  retval  = __ieee754_y1(num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *_errno() = EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

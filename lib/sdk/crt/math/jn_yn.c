#include <math.h>
#include <float.h>
#include "ieee754/ieee754.h"

typedef int fpclass_t;
fpclass_t _fpclass(double __d);
int *_errno(void);

/*
 * @unimplemented
 */
double _jn(int n, double num)
{
  /* FIXME: errno handling */
  return __ieee754_jn(n, num);
}

/*
 * @implemented
 */
double _yn(int order, double num)
{
  double retval;
  if (!_finite(num)) *_errno() = EDOM;
  retval  = __ieee754_yn(order,num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *_errno() = EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

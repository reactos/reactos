#include <math.h>
#include <float.h>
#include "ieee754/ieee754.h"

int *_errno(void);

/*
 * @unimplemented
 */
double _j0(double num)
{
  if (!_finite(num)) *_errno() = EDOM;
  return __ieee754_j0(num);
}

/*
 * @implemented
 */
double _y0(double num)
{
  double retval;
  int fpclass = _fpclass(num);

  if (!_finite(num) || fpclass == _FPCLASS_NN ||
      fpclass == _FPCLASS_ND || fpclass == _FPCLASS_NZ)
    *_errno() = EDOM;

  retval  = __ieee754_y0(num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *_errno() = EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

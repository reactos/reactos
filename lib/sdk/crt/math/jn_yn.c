#include <math.h>

typedef int fpclass_t;
fpclass_t _fpclass(double __d);
int *_errno(void);

/*
 * @unimplemented
 */
double _jn(int n, double num)
{
  /* FIXME: errno handling */
  return jn(n, num);
}

/*
 * @implemented
 */
double _yn(int order, double num)
{
  double retval;
  if (!isfinite(num)) *_errno() = EDOM;
  retval  = yn(order,num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *_errno() = EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

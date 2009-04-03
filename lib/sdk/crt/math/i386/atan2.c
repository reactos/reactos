
#include <math.h>

double atan2 (double __y, double __x);

/*
 * @implemented
 */
double atan2 (double __y, double __x)
{
  register double __val;
#ifdef __GNUC__
  __asm __volatile__
    ("fpatan\n\t"
     "fld %%st(0)"
     : "=t" (__val) : "0" (__x), "u" (__y));
#else
  __val = linkme_atan2(__x, __y);
#endif /*__GNUC__*/
  return __val;
}

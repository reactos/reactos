
#include <msvcrt/math.h>

double atan2 (double __y, double __x);

/*
 * @implemented
 */
double atan2 (double __y, double __x)
{
  register double __value;
#ifdef __GNUC__
  __asm __volatile__
    ("fpatan\n\t"
     "fld %%st(0)"
     : "=t" (__value) : "0" (__x), "u" (__y));
#else
  __value = linkme_atan2(__x, __y);
#endif /*__GNUC__*/
  return __value;
}

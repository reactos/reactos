#include <msvcrt/math.h>

double cos (double __x);

double cos (double __x)
{
  register double __value;
#ifdef __GNUC__
  __asm __volatile__
    ("fcos"
     : "=t" (__value): "0" (__x));
#else
  __value = linkme_cos(__x);
#endif /*__GNUC__*/
  return __value;
}

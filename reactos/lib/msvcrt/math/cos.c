#include <msvcrti.h>


double cos (double __x);

double cos (double __x)
{
  register double __value;
  __asm __volatile__
    ("fcos"
     : "=t" (__value): "0" (__x));

  return __value;
}

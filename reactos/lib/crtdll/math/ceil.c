#include <msvcrt/math.h>

double ceil (double __x)
{
  register double __value;
#ifdef __GNUC__
  __volatile unsigned short int __cw, __cwtmp;

  __asm __volatile ("fnstcw %0" : "=m" (__cw));
  __cwtmp = (__cw & 0xf3ff) | 0x0800; /* rounding up */
  __asm __volatile ("fldcw %0" : : "m" (__cwtmp));
  __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));
  __asm __volatile ("fldcw %0" : : "m" (__cw));
#else
  __value = linkme_ceil(__x);
#endif /*__GNUC__*/
  return __value;
}


long double ceill (long double __x)
{
	return floor(__x)+1;
}

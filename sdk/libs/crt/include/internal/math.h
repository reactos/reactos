#ifndef __CRT_INTERNAL_MATH_H
#define __CRT_INTERNAL_MATH_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

int     _isinf          (double); /* not exported */
int     _isnanl         (long double); /* not exported */
int     _isinfl         (long double); /* not exported */

#endif

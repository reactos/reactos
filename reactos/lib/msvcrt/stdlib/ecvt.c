/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


char *ecvtbuf (double, int, int *, int *, char *);

char *
_ecvt (double value, int ndigits, int *decpt, int *sign)
{
  static char ecvt_buf[DBL_MAX_10_EXP + 10];
  return ecvtbuf (value, ndigits, decpt, sign, ecvt_buf);
}

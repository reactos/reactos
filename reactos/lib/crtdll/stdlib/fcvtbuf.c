/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <locale.h>

void __ecvround (char *, char *, const char *, int *);

char *
fcvtbuf (double value, int ndigits, int *decpt, int *sign, char *buf)
{
  static char INFINITY[] = "Infinity";
  char decimal = localeconv()->decimal_point[0];
  int digits = ndigits >= 0 ? ndigits : 0;
  char *cvtbuf = (char *)alloca (2*DBL_MAX_10_EXP + 16);
  char *s = cvtbuf;
  char *dot;

  sprintf (cvtbuf, "%-+#.*f", DBL_MAX_10_EXP + digits + 1, value);

  /* The sign.  */
  if (*s++ == '-')
    *sign = 1;
  else
    *sign = 0;

  /* Where's the decimal point?  */
  dot = strchr (s, decimal);
  *decpt = dot ? dot - s : strlen (s);

  /* SunOS docs says if NDIGITS is 8 or more, produce "Infinity"
     instead of "Inf".  */
  if (strncmp (s, "Inf", 3) == 0)
    {
      memcpy (buf, INFINITY, ndigits >= 8 ? 9 : 3);
      if (ndigits < 8)
	buf[3] = '\0';
      return buf;
    }
  else if (ndigits < 0)
    return ecvtbuf (value, *decpt + ndigits, decpt, sign, buf);
  else if (*s == '0' && value != 0.0)
    return ecvtbuf (value, ndigits, decpt, sign, buf);
  else
    {
      memcpy (buf, s, *decpt);
      if (s[*decpt] == decimal)
	{
	  memcpy (buf + *decpt, s + *decpt + 1, ndigits);
	  buf[*decpt + ndigits] = '\0';
	}
      else
	buf[*decpt] = '\0';
      __ecvround (buf, buf + *decpt + ndigits - 1,
		s + *decpt + ndigits + 1, decpt);
      return buf;
    }
}

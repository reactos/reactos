/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


void __ecvround (char *, char *, const char *, int *);

void
__ecvround (char *numbuf, char *last_digit, const char *after_last, int *decpt)
{
  char *p;
  int carry = 0;

  /* Do we have at all to round the last digit?  */
  if (*after_last > '4')
    {
      p = last_digit;
      carry = 1;

      /* Propagate the rounding through trailing '9' digits.  */
      do {
	int sum = *p + carry;
	carry = sum > '9';
	*p-- = sum - carry * 10;
      } while (carry && p >= numbuf);

      /* We have 9999999... which needs to be rounded to 100000..  */
      if (carry && p == numbuf)
	{
	  *p = '1';
	  *decpt += 1;
	}
    }
}

char *
ecvtbuf (double value, int ndigits, int *decpt, int *sign, char *buf)
{
  static char _INFINITY[] = "Infinity";
  char decimal = '.' /* localeconv()->decimal_point[0] */;
  char *cvtbuf = (char *)alloca (ndigits + 20); /* +3 for sign, dot, null; */
					        /* two extra for rounding */
						/* 15 extra for alignment */
  char *s = cvtbuf, *d = buf;

  /* Produce two extra digits, so we could round properly.  */
  sprintf (cvtbuf, "%-+.*E", ndigits + 2, value);
  *decpt = 0;

  /* The sign.  */
  if (*s++ == '-')
    *sign = 1;
  else
    *sign = 0;

  /* Special values get special treatment.  */
  if (strncmp (s, "Inf", 3) == 0)
    {
      /* SunOS docs says we have return "Infinity" for NDIGITS >= 8.  */
      memcpy (buf, _INFINITY, ndigits >= 8 ? 9 : 3);
      if (ndigits < 8)
	buf[3] = '\0';
    }
  else if (strcmp (s, "NaN") == 0)
    memcpy (buf, s, 4);
  else
    {
      char *last_digit, *digit_after_last;

      /* Copy (the single) digit before the decimal.  */
      while (*s && *s != decimal && d - buf < ndigits)
	*d++ = *s++;

      /* If we don't see any exponent, here's our decimal point.  */
      *decpt = d - buf;
      if (*s)
	s++;

      /* Copy the fraction digits.  */
      while (*s && *s != 'E' && d - buf < ndigits)
	*d++ = *s++;

      /* Remember the last digit copied and the one after it.  */
      last_digit = d > buf ? d - 1 : d;
      digit_after_last = s;

      /* Get past the E in exponent field.  */
      while (*s && *s++ != 'E')
	;

      /* Adjust the decimal point by the exponent value.  */
      *decpt += atoi (s);

      /* Pad with zeroes if needed.  */
      while (d - buf < ndigits)
	*d++ = '0';

      /* Zero-terminate.  */
      *d = '\0';

      /* Round if necessary.  */
      __ecvround (buf, last_digit, digit_after_last, decpt);
    }
  return buf;
}

/*
 * PROJECT:         ReactOS CRT
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         CRT's ecvt
 * FILE:            lib/sdk/crt/stdlib/ecvt.c
 * PROGRAMERS:      Gregor Schneider (parts based on ecvtbuf.c by DJ Delorie)
 */

#include <precomp.h>
#define NUMBER_EFMT 18 /* sign, dot, null, 15 for alignment */

/*
 * @implemented
 */
char *
_ecvt (double value, int ndigits, int *decpt, int *sign)
{
  static char ecvtbuf[DBL_MAX_10_EXP + 10];
  char *cvtbuf, *s, *d;

  if (ndigits < 0) ndigits = 0;
  s = cvtbuf = (char*)malloc(ndigits + NUMBER_EFMT);
  d = ecvtbuf;

  *sign = 0;
  *decpt = 0;

  if (cvtbuf == NULL)
  {
    return NULL;
  }

  sprintf(cvtbuf, "%-+.*E", ndigits, value);
  /* Treat special values */
  if (strncmp(s, "NaN", 3) == 0)
  {
    memcpy(ecvtbuf, s, 4);
  }
  else if (strncmp(s + 1, "Inf", 3) == 0)
  {
    memcpy(ecvtbuf, s, 5);
  }
  else
  {
    /* Set the sign */
    if (*s && *s == '-')
    {
      *sign = 1;
    }
    s++;
    /* Copy the first digit */
    if (*s && *s != '.')
    {
      if (d - ecvtbuf < ndigits)
      {
        *d++ = *s++;
      }
      else
      {
        s++;
      }
    }
    /* Skip the decimal point */
    if (*s && *s == '.')
    {
      s++;
    }
    /* Copy fractional digits */
    while (*s && *s != 'E')
    {
      if (d - ecvtbuf < ndigits)
      {
        *d++ = *s++;
      }
      else
      {
        s++;
      }
    }
    /* Skip the exponent */
    if (*s && *s == 'E')
    {
      s++;
    }
    /* Set the decimal point to the exponent value plus the one digit we copied */
    *decpt = atoi(s) + 1;
    /* Handle special decimal point cases */
    if (cvtbuf[1] == '0')
    {
      *decpt = 0;
    }
    if (ndigits < 1)
    {
      /* Need enhanced precision*/
      char* tbuf = (char*)malloc(NUMBER_EFMT);
      if (tbuf == NULL)
      {
        free(cvtbuf);
        return NULL;
      }
      sprintf(tbuf, "%-+.*E", ndigits + 2, value);
      if (tbuf[1] >= '5')
      {
        (*decpt)++;
      }
      free(tbuf);
    }
    /* Pad with zeroes */
    while (d - ecvtbuf < ndigits)
    {
      *d++ = '0';
    }
    /* Terminate */
    *d = '\0';
  }
  free(cvtbuf);
  return ecvtbuf;
}

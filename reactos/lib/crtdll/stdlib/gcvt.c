/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>

/*
 * @implemented
 */
char *
_gcvt (double value, int ndigits, char *buf)
{
  char *p = buf;

  sprintf (buf, "%-#.*g", ndigits, value);

  /* It seems they expect us to return .XXXX instead of 0.XXXX  */
  if (*p == '-')
    p++;
  if (*p == '0' && p[1] == '.')
    memmove (p, p + 1, strlen (p + 1) + 1);

  /* They want Xe-YY, not X.e-YY, and XXXX instead of XXXX.  */
  p = strchr (buf, 'e');
  if (!p)
    {
      p = buf + strlen (buf);
      /* They don't want trailing zeroes.  */
      while (p[-1] == '0' && p > buf + 2)
	*--p = '\0';
    }
  if (p > buf && p[-1] == '.')
    memmove (p - 1, p, strlen (p) + 1);
  return buf;
}

/* $Id: stdlib.c,v 1.7 2002/09/08 10:23:42 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/stdlib.c
 * PURPOSE:         Standard library functions
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *   1999/07/29  ekohl   Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* GLOBALS   ****************************************************************/

static unsigned long long next = 0;

/* FUNCTIONS ****************************************************************/

int atoi(const char *str)
{
	return (int)atol (str);
}


/*
 * NOTE: no error 
 */

long atol(const char *str)
{
  const char *s = str;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * Skip white space and pick up leading +/- sign if any.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;

  /*
   * Compute the cutoff value between legal numbers and illegal
   * numbers.  That is the largest legal value, divided by the
   * base.  An input number that is greater than this value, if
   * followed by a legal input character, is too big.  One that
   * is equal to this value may be valid or not; the limit
   * between valid and invalid numbers is then based on the last
   * digit.  For instance, if the range for longs is
   * [-2147483648..2147483647] and the input base is 10,
   * cutoff will be set to 214748364 and cutlim to either
   * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
   * a value > 214748364, or equal but the next digit is > 7 (or 8),
   * the number is too big, and we will return a range error.
   *
   * Set any if any `digits' consumed; make it negative to indicate
   * overflow.
   */
  cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
  cutlim = cutoff % (unsigned long)10;
  cutoff /= (unsigned long)10;
  for (acc = 0, any = 0;; c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else
      break;
    if (c >= 10)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else
    {
      any = 1;
      acc *= 10;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = neg ? LONG_MIN : LONG_MAX;
  }
  else if (neg)
    acc = -acc;

  return acc;
}


/*
 * NOTE: no radix range check (valid range: 2 - 36)
 */

char *_itoa (int value, char *string, int radix)
{
  char tmp[33];
  char *tp = tmp;
  int i;
  unsigned v;
  int sign;
  char *sp = NULL;

  if (string == NULL)
    return NULL;

  sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned)value;

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  if (sign)
    *sp++ = '-';

  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;

  return string;
}


int rand(void)
{
	next = next * 0x5deece66dLL + 11;
	return (int)((next >> 16) & RAND_MAX);
}


void srand(unsigned seed)
{
	next = seed;
}


int mbtowc (wchar_t *wchar, const char *mbchar, size_t count)
{
	NTSTATUS Status;
	ULONG Size;

	if (wchar == NULL)
		return 0;

	Status = RtlMultiByteToUnicodeN (wchar,
	                                 sizeof(WCHAR),
	                                 &Size,
	                                 (char *)mbchar,
	                                 count);
	if (!NT_SUCCESS(Status))
		return -1;

	return (int)Size;
}


size_t mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = strlen (mbstr);

	if (wcstr == NULL)
	{
		RtlMultiByteToUnicodeSize (&Size,
		                           (char *)mbstr,
		                           Length);

		return (size_t)Size;
	}

	Status = RtlMultiByteToUnicodeN (wcstr,
	                                 count,
	                                 &Size,
	                                 (char *)mbstr,
	                                 Length);
	if (!NT_SUCCESS(Status))
		return -1;

	return (size_t)Size;
}


int wctomb (char *mbchar, wchar_t wchar)
{
	NTSTATUS Status;
	ULONG Size;

	if (mbchar == NULL)
		return 0;

	Status = RtlUnicodeToMultiByteN (mbchar,
	                                 1,
	                                 &Size,
	                                 &wchar,
	                                 sizeof(WCHAR));
	if (!NT_SUCCESS(Status))
		return -1;

	return (int)Size;
}


size_t wcstombs (char *mbstr, const wchar_t *wcstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = wcslen (wcstr);

	if (mbstr == NULL)
	{
		RtlUnicodeToMultiByteSize (&Size,
		                           (wchar_t *)wcstr,
		                           Length * sizeof(WCHAR));

		return (size_t)Size;
	}

	Status = RtlUnicodeToMultiByteN (mbstr,
	                                 count,
	                                 &Size,
	                                 (wchar_t *)wcstr,
	                                 Length * sizeof(WCHAR));
	if (!NT_SUCCESS(Status))
		return -1;

	return (size_t)Size;
}

/* EOF */

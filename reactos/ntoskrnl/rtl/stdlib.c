/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/stdlib.c
 * PURPOSE:         Standard library functions
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *   1999/07/29  ekohl   Created
 */

/* INCLUDES *****************************************************************/

#include <ctype.h>
#include <stdlib.h>

/* GLOBALS   ****************************************************************/

static unsigned long long next = 0;

/* FUNCTIONS ****************************************************************/

#if 0
int atoi(const char *str)
{
	return (int)atol (str);
}

long atol(const char *str)
{
	return strtol(str, 0, 10);
}
#endif


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

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdlib.h>

void _swab (const char* caFrom, char* caTo, size_t sizeToCopy)
{
  unsigned long temp;

  sizeToCopy >>= 1; sizeToCopy++;
#define	STEP	temp = *((const char *)caFrom)++,*((char *)caTo)++ = *((const char *)caFrom)++,*((char *)caTo)++ = temp
  /* round to multiple of 8 */
  while ((--sizeToCopy) & 07)
    STEP;
  sizeToCopy >>= 3;
  while (--sizeToCopy >= 0) {
    STEP; STEP; STEP; STEP;
    STEP; STEP; STEP; STEP;
  }
}

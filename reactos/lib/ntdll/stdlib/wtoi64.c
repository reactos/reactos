/* $Id: wtoi64.c,v 1.1 1999/12/30 01:32:35 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/stdlib/wtoi64.c
 * PURPOSE:         converts a unicode string to 64 bit integer
 */

#include <stdlib.h>
#include <ctype.h>

__int64
_wtoi64 (const wchar_t *nptr)
{
   int c;
   __int64 value;
   int sign;

   while (iswctype((int)*nptr, _SPACE))
        ++nptr;

   c = (int)*nptr++;
   sign = c;
   if (c == L'-' || c == L'+')
        c = (int)*nptr++;

   value = 0;

   while (iswctype(c, _DIGIT))
     {
        value = 10 * value + (c - L'0');
        c = (int)*nptr++;
     }

   if (sign == L'-')
       return -value;
   else
       return value;
}

/* EOF */
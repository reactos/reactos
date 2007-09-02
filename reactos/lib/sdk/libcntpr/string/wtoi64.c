#include <string.h>
#include <ctype.h>
#include <basetsd.h>

/*
 * @implemented
 */
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

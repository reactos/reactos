#include <string.h>
#include <ctype.h>
#include <basetsd.h>

/*
 * @implemented
 */
__int64
_atoi64 (const char *nptr)
{
   int c;
   __int64 value;
   int sign;

   while (isspace((int)*nptr))
        ++nptr;

   c = (int)*nptr++;
   sign = c;
   if (c == '-' || c == '+')
        c = (int)*nptr++;

   value = 0;

   while (isdigit(c))
     {
        value = 10 * value + (c - '0');
        c = (int)*nptr++;
     }

   if (sign == '-')
       return -value;
   else
       return value;
}

/* EOF */

/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */


/* These routines are fast memcpy, memset routines.  When available, I
   use system rouines.  For msdos, I use inline assembly. */

/* The current versions only work in the forward direction only!! */
      
#include "config.h"

#include <stdio.h>
#include "slang.h"
#include "_slang.h"

char *SLmemcpy(char *s1, char *s2, int n)
{
#if defined(msdos) && !defined(__WIN32__) && !defined(__GO32__)
   asm mov ax, ds
   asm mov bx, si
   asm mov dx, di
   asm mov cx, n
   asm les di, s1
   asm lds si, s2
   asm cld
   asm rep movsb
   asm mov ds, ax
   asm mov si, bx
   asm mov di, dx
   return(s1);
   
#else
   register char *smax, *s = s1;
   int n2;
   
   n2 = n % 4;
   smax = s + (n - 4);
   while (s <= smax) 
     {
	*s = *s2; *(s + 1) = *(s2 + 1); *(s + 2) = *(s2 + 2); *(s + 3) = *(s2 + 3);
	s += 4;
	s2 += 4;
     }
   while (n2--) *s++ = *s2++;
   return(s1); 
#endif
}

/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */

#include "config.h"

#include <stdio.h>
#include "slang.h"
#include "_slang.h"

unsigned int SLang_Input_Buffer_Len = 0;
unsigned char SLang_Input_Buffer [MAX_INPUT_BUFFER_LEN];

int SLang_Abort_Char = 7;
int SLang_Ignore_User_Abort = 0;

/* This has the effect of mapping all characters in the range 128-169 to
 * ESC [ something 
 */
#ifndef __GO32__
# if defined(__unix__) || defined(vms)
#  define DEC_8BIT_HACK 64
# endif
#endif

#ifdef OS2_NT /* see the replacement in src/slint.c */
unsigned int SLang_getkey (void)
{
   unsigned int imax;
   unsigned int ch;
   
   if (SLang_Input_Buffer_Len)
     {
	ch = (unsigned int) *SLang_Input_Buffer;
	SLang_Input_Buffer_Len--;
	imax = SLang_Input_Buffer_Len;
   
	SLMEMCPY ((char *) SLang_Input_Buffer, 
		(char *) (SLang_Input_Buffer + 1), imax);
     }
   else if (0xFFFF == (ch = SLsys_getkey ())) return ch;
   
#ifdef DEC_8BIT_HACK
   if (ch & 0x80)
     {
	unsigned char i;
	i = (unsigned char) (ch & 0x7F);
	if (i < ' ')
	  {
	     i += DEC_8BIT_HACK;
	     SLang_ungetkey (i);
	     ch = 27;
	  }
     }
#endif
   return(ch);
}
#endif /* OS2_NT */

void SLang_ungetkey_string (unsigned char *s, unsigned int n)
{
   register unsigned char *bmax, *b, *b1;
   if (SLang_Input_Buffer_Len + n + 3 > MAX_INPUT_BUFFER_LEN) return;

   b = SLang_Input_Buffer;
   bmax = (b - 1) + SLang_Input_Buffer_Len;
   b1 = bmax + n;
   while (bmax >= b) *b1-- = *bmax--;
   bmax = b + n;
   while (b < bmax) *b++ = *s++;
   SLang_Input_Buffer_Len += n;
}

void SLang_buffer_keystring (unsigned char *s, unsigned int n)
{

   if (n + SLang_Input_Buffer_Len + 3 > MAX_INPUT_BUFFER_LEN) return;
   
   SLMEMCPY ((char *) SLang_Input_Buffer + SLang_Input_Buffer_Len, 
	   (char *) s, n);
   SLang_Input_Buffer_Len += n;
}

void SLang_ungetkey (unsigned char ch)
{
   SLang_ungetkey_string(&ch, 1);
}

#ifdef OS2_NT /* see the replacement in src/slint.c */
int SLang_input_pending (int tsecs)
{
   int n;
   unsigned char c;
   if (SLang_Input_Buffer_Len) return (int) SLang_Input_Buffer_Len;
   
   n = SLsys_input_pending (tsecs);
   
   if (n <= 0) return 0;
   
   c = (unsigned char) SLang_getkey ();
   SLang_ungetkey_string (&c, 1);
   
   return n;
}
#endif /* OS2_NT */

void SLang_flush_input (void)
{
   int quit = SLKeyBoard_Quit;
   
   SLang_Input_Buffer_Len = 0;
   SLKeyBoard_Quit = 0;   
   while (SLsys_input_pending (0) > 0) 
     {
	(void) SLsys_getkey ();
	/* Set this to 0 because SLsys_getkey may stuff keyboard buffer if
	 * key sends key sequence (OS/2, DOS, maybe VMS).
	 */
	SLang_Input_Buffer_Len = 0;
     }
   SLKeyBoard_Quit = quit;
}

/*--------------------------------*-C-*---------------------------------*
 * File:	sltoken.c
 *
 * Descript:	---
 *
 * Requires:	---
 *
 * Public:	SLexpand_escaped_char ();
 *		SLexpand_escaped_string ();
 *		SLang_extract_token ();
 *		SLang_guess_type ();
 *		SLatoi ();
 *
 * Private:	---
 *
 * Notes:	---
 *
 * Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
\*----------------------------------------------------------------------*/

#include "config.h"

#include <stdio.h>


#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <string.h>
#include "slang.h"
#include "_slang.h"

/* There are non-zeros at positions "\t %()*,/:;[]{}" */

static unsigned char special_chars[256] =
{
   /* 0 */	0,0,0,0,0,0,0,0,	0,'\t',0,0,0,0,0,0,
   /* 16 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 32 */	' ',0,0,0,0,'%',0,0,	'(',')','*',0,',',0,0,'/',
   /* 48 */	0,0,0,0,0,0,0,0,	0,0,':',';',0,0,0,0,
   /* 64 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 80 */	0,0,0,0,0,0,0,0,	0,0,0,'[',0,']',0,0,
   /* 96 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 112 */	0,0,0,0,0,0,0,0,	0,0,0,'{',0,'}',0,0,
   /* 8-bit characters */
   /* 128 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 144 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 160 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 176 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 192 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 208 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 224 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
   /* 240 */	0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0
};

char *SLexpand_escaped_char(char *p, char *ch)
{
   int i = 0;
   int max = 0, num, base = 0;
   char ch1;
   
   ch1 = *p++;
   
   switch (ch1)
     {
      default: num = ch1; break;
      case 'n': num = '\n'; break;
      case 't': num = '\t'; break;
      case 'v': num = '\v'; break;
      case 'b': num = '\b'; break;
      case 'r': num = '\r'; break;
      case 'f': num = '\f'; break;
      case 'E': case 'e': num = 27; break;
      case 'a': num = 7; 
	break;
	
	/* octal */
      case '0': case '1': case '2': case '3': 
      case '4': case '5': case '6': case '7': 
	max = '7'; 
	base = 8; i = 2; num = ch1 - '0';
	break;
	
      case 'd':			       /* decimal -- S-Lang extension */
	base = 10; 
	i = 3;
	max = '9';
	num = 0;
	break;
	
      case 'x':			       /* hex */
	base = 16;
	max = '9';
	i = 2;
	num = 0;
	break;
     }
   
   while (i--)
     {
	ch1 = *p;
	
	if ((ch1 <= max) && (ch1 >= '0'))
	  {
	     num = base * num + (ch1 - '0');
	  }
	else if (base == 16)
	  {
	     ch1 |= 0x20;
	     if ((ch1 < 'a') || ((ch1 > 'f'))) break;
	     num = base * num + 10 + (ch1 - 'a');
	  }
	else break;
	p++;
     }
   
   *ch = (char) num;
   return p;
}

void SLexpand_escaped_string (register char *s, register char *t, 
			      register char *tmax)
{
   char ch;
   
   while (t < tmax)
     {
	ch = *t++;
	if (ch == '\\')
	  {
	     t = SLexpand_escaped_char (t, &ch);
	  }
	*s++ = ch;
     }
   *s = 0;
}


int SLang_extract_token (char **linep, char *word_parm, int byte_comp)
{
   register char ch, *line, *word = word_parm;
   int string;
   char ch1;
   char *word_max;
   
   word_max = word + 250;
   
   line = *linep;

   /* skip white space */
   while (((ch = *line) == ' ') 
	  || (ch == '\t')) line++;

   if ((!ch) || (ch == '\n'))
     {
	*linep = line;
	return(0);
     }
   
   *word++ = ch;
   line++;
   
   /* Look for -something and rule out --something and -= something */
   if ((ch == '-') && 
       (*line != '-') && (*line != '=') && ((*line > '9') || (*line < '0')))
     {
	*word = 0;
	*linep = line;
	return 1;
     }
   
       
   if (ch == '"') string = 1; else string = 0;
   if (ch == '\'')
     {
	if ((ch = *line++) != 0)
	  {
	     if (ch == '\\') 
	       {
		  line = SLexpand_escaped_char(line, &ch1);
		  ch = ch1;
	       }
	     if (*line++ == '\'')
	       {
		  --word;
		  sprintf(word, "%d", (int) ((unsigned char) ch));
		  word += strlen (word);  ch = '\'';
	       }
	     else SLang_Error = SYNTAX_ERROR;
	  }
	else SLang_Error = SYNTAX_ERROR;
     }
   else  if (!special_chars[(unsigned char) ch])
     {
	while (ch = *line++, 
	       (ch > '"') || 
	       ((ch != '\n') && (ch != 0) && (ch != '"')))
	  {
	     if (string)
	       {
		  if (ch == '\\')
		    {
		       ch = *line++;
		       if ((ch == 0) || (ch == '\n')) break;
		       if (byte_comp) *word++ = '\\';
		       else 
			 {
			    line = SLexpand_escaped_char(line - 1, &ch1);
			    ch = ch1;
			 }
		    }
	       }
	     else if (special_chars[(unsigned char) ch])
	       {
		  line--;
		  break;
	       }
	     
	     *word++ = ch;
	     if (word > word_max)
	       {
		  SLang_doerror ("Token to large.");
		  break;
	       }
	  }
     }
   
   if ((!ch) || (ch == '\n')) line--;
   if ((ch == '"') && string) *word++ = '"'; else if (string) SLang_Error = SYNTAX_ERROR;
   *word = 0;
   *linep = line;
   /* massage variable-- and ++ into --variable, etc... */
   if (((int) (word - word_parm) > 2)
       && (ch = *(word - 1), (ch == '+') || (ch == '-'))
       && (ch == *(word - 2)))
     {
	word--;
	while (word >= word_parm + 2)
	  {
	     *word = *(word - 2);
	     word--;
	  }
	*word-- = ch;
	*word-- = ch;
     }
   return(1);
}


int SLang_guess_type (char *t)
{
   char *p;
   register char ch;

   if (*t == '-') t++;
   p = t;
#ifdef FLOAT_TYPE
   if (*p != '.') 
     {
#endif
	while ((*p >= '0') && (*p <= '9')) p++;
	if (t == p) return(STRING_TYPE);
	if ((*p == 'x') && (p == t + 1))   /* 0x?? */
	  {
	     p++;
	     while (ch = *p, 
		    ((ch >= '0') && (ch <= '9'))
		    || (((ch | 0x20) >= 'a') && ((ch | 0x20) <= 'f'))) p++;
	  }
	if (*p == 0) return(INT_TYPE);
#ifndef FLOAT_TYPE
	return(STRING_TYPE);
#else
     }
   
   /* now down to float case */
   if (*p == '.')
     {
	p++;
	while ((*p >= '0') && (*p <= '9')) p++;
     }
   if (*p == 0) return(FLOAT_TYPE);
   if ((*p != 'e') && (*p != 'E')) return(STRING_TYPE);
   p++;
   if ((*p == '-') || (*p == '+')) p++;
   while ((*p >= '0') && (*p <= '9')) p++;
   if (*p != 0) return(STRING_TYPE); else return(FLOAT_TYPE);
#endif
}

int SLatoi (unsigned char *s)
{
   register unsigned char ch;
   register unsigned int value;
   register int base;
   
   if (*s != '0') return atoi((char *) s);

   /* look for 'x' which indicates hex */
   s++;
   if ((*s | 0x20) == 'x') 
     {
	base = 16;
	s++;
	if (*s == 0) 
	  {
	     SLang_Error = SYNTAX_ERROR;
	     return -1;
	  }
     }
   else base = 8;
   
   
   value = 0;
   while ((ch = *s++) != 0)
     {
	char ch1 = ch | 0x20;
	switch (ch1)
	  {
	   default:
	     SLang_Error = SYNTAX_ERROR;
	     break;
	   case '8':
	   case '9':
	     if (base != 16) SLang_Error = SYNTAX_ERROR;
	     /* drop */
	   case '0':
	   case '1':
	   case '2':
	   case '3':
	   case '4':
	   case '5':
	   case '6':
	   case '7':
	     ch1 -= '0';
	     break;
	     
	   case 'a':
	   case 'b':
	   case 'c':
	   case 'd':
	   case 'e':
	   case 'f':
	     if (base != 16) SLang_Error = SYNTAX_ERROR;
	     ch1 = (ch1 - 'a') + 10;
	     break;
	  }
	value = value * base + ch1;
     }
   return (int) value;
}

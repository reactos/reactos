/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/strtok.c
 * PURPOSE:         Unicode and thread safe implementation of strtok
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

char* strtok(char *s, const char *delim)
{
  const char *spanp;
  int c, sc;
  char *tok;
  static char *last;
   
  if (s == NULL && (s = last) == NULL)
    return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
 cont:
  c = *s++;
  for (spanp = delim; (sc = *spanp++) != 0;) {
    if (c == sc)
      goto cont;
  }

  if (c == 0) {			/* no non-delimiter characters */
    last = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
	if (c == 0)
	  s = NULL;
	else
	  s[-1] = 0;
	last = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}

PWSTR RtlStrtok(PUNICODE_STRING _string, PWSTR _sep,
		PWSTR* temp)
/*
 * FUNCTION: Splits a string into tokens
 * ARGUMENTS:
 *         string = string to operate on
 *                  if NULL then continue with previous string
 *         sep = Token deliminators
 *         temp = Tempory storage provided by the caller
 * ARGUMENTS: Returns the beginning of the next token
 */
{
   PWSTR string;
   PWSTR sep;
   PWSTR start;
   
   if (_string!=NULL)
     {
	string = _string->Buffer;
     }
   else
     {
	string = *temp;
     }
   
   start = string;
   
   while ((*string)!=0)
     {
	sep = _sep;
	while ((*sep)!=0)
	  {
	     if ((*string)==(*sep))
	       {
		  *string=0;
		  *temp=string+1;
		  return(start);
	       }
	     sep++;
	  }
	string++;
     }
   *temp=NULL;
   return(start);
}

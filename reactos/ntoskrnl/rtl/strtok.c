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

#include <ddk/ntddk.h>

#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

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

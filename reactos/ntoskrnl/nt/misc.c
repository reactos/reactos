/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/misc.c
 * PURPOSE:         Misc undocumented system calls
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtDisplayString(IN PUNICODE_STRING DisplayString)
{
   ANSI_STRING AnsiString;

   RtlUnicodeStringToAnsiString (&AnsiString, DisplayString, TRUE);

   HalDisplayString (AnsiString.Buffer);

   RtlFreeAnsiString (&AnsiString);

   return(STATUS_SUCCESS);
}

/* EOF */

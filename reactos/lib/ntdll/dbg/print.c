/* $Id: print.c,v 1.5 2002/09/07 15:12:39 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/dbg/print.c
 * PURPOSE:         Debug output
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  Created 28/12/1999
 */

/* INCLUDES ***************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

ULONG DbgService (ULONG Service, PVOID Context1, PVOID Context2);
__asm__ ("\n\t.global _DbgService\n\t"
         "_DbgService:\n\t"
         "mov 4(%esp), %eax\n\t"
         "mov 8(%esp), %ecx\n\t"
         "mov 12(%esp), %edx\n\t"
         "int $0x2D\n\t"
         "ret\n\t");

ULONG
DbgPrint(PCH Format, ...)
{
   ANSI_STRING DebugString;
   CHAR Buffer[4096];
   va_list ap;

   /* init ansi string */
   DebugString.Buffer = Buffer;
   DebugString.MaximumLength = sizeof(Buffer);

   va_start (ap, Format);
   DebugString.Length = _vsnprintf (Buffer, sizeof(Buffer), Format, ap);
   va_end (ap);

   DbgService (1, &DebugString, NULL);

   return (ULONG)DebugString.Length;
}


VOID
STDCALL
DbgPrompt (
	PCH OutputString,
	PCH InputString,
	USHORT InputSize
	)
{
	ANSI_STRING Output;
	ANSI_STRING Input;

	Input.Length = 0;
	Input.MaximumLength = InputSize;
	Input.Buffer = InputString;

	Output.Length = strlen (OutputString);
	Output.MaximumLength = Output.Length + 1;
	Output.Buffer = OutputString;

	DbgService (2,
	            &Output,
	            &Input);
}

/* EOF */

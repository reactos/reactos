/* $Id: print.c,v 1.10 2000/10/22 16:36:49 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/print.c
 * PURPOSE:         Debug output 
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  14/10/99: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/kd.h>


/* FUNCTIONS ****************************************************************/

#if 0
ULONG DbgService (ULONG Service, PVOID Context1, PVOID Context2);
__asm__ ("\n\t.global _DbgService\n\t"
         "_DbgService:\n\t"
         "mov 4(%esp), %eax\n\t"
         "mov 8(%esp), %ecx\n\t"
         "mov 12(%esp), %edx\n\t"
         "int $0x2D\n\t"
         "ret\n\t");
#endif

/*
 * Note: DON'T CHANGE THIS FUNCTION!!!
 *       DON'T CALL HalDisplayString OR SOMETING ELSE!!!
 *       You'll only break the serial/bochs debugging feature!!!
 */

ULONG DbgPrint(PCH Format, ...)
{
   ANSI_STRING DebugString;
   CHAR Buffer[512];
   va_list ap;

   /* init ansi string */
   DebugString.Buffer = Buffer;
   DebugString.MaximumLength = 512;

   va_start (ap, Format);
   DebugString.Length = vsprintf (Buffer, Format, ap);
   va_end (ap);

   KdpPrintString (&DebugString);

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

	/* FIXME: Not implemented yet! */
//	KdpPromptString (&Output,
//	                 &Input);
}

/* EOF */

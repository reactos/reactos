/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: print.c,v 1.15 2002/09/07 15:12:49 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/print.c
 * PURPOSE:         Debug output 
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * PORTABILITY:     Unchecked
 * UPDATE HISTORY:
 *                  14/10/99: Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


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

ULONG 
DbgPrint(PCH Format, ...)
{
   ANSI_STRING DebugString;
   CHAR Buffer[1024];
   va_list ap;

   /* init ansi string */
   DebugString.Buffer = Buffer;
   DebugString.MaximumLength = sizeof(Buffer);

   va_start (ap, Format);
   DebugString.Length = _vsnprintf (Buffer, sizeof( Buffer ), Format, ap);
   va_end (ap);

   KdpPrintString (&DebugString);

   return (ULONG)DebugString.Length;
}


VOID STDCALL
DbgPrompt (PCH OutputString,
	   PCH InputString,
	   USHORT InputSize)
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

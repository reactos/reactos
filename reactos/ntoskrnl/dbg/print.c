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
/* $Id$
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

/*
 * @implemented
 */
ULONG 
DbgPrint(PCH Format, ...)
{
   ANSI_STRING DebugString;
   CHAR Buffer[1024];
   va_list ap;
#ifdef SERIALIZE_DBGPRINT
#  define MESSAGETABLE_SIZE  16
   LONG MyTableIndex;
   static LONG Lock = 0;
   static LONG TableWriteIndex = 0, TableReadIndex = 0;
   static CHAR MessageTable[MESSAGETABLE_SIZE][sizeof(Buffer)] = { { '\0' } };
#endif /* SERIALIZE_DBGPRINT */

   /* init ansi string */
   DebugString.Buffer = Buffer;
   DebugString.MaximumLength = sizeof(Buffer);

   va_start (ap, Format);
   DebugString.Length = _vsnprintf (Buffer, sizeof( Buffer ), Format, ap);
   va_end (ap);

#ifdef SERIALIZE_DBGPRINT
   /* check if we are already running */
   if (InterlockedCompareExchange(&Lock, 1, 0) == 1)
     {
        MyTableIndex = InterlockedIncrement(&TableWriteIndex) - 1;
        InterlockedCompareExchange(&TableWriteIndex, 0, MESSAGETABLE_SIZE);
        MyTableIndex %= MESSAGETABLE_SIZE;

        if (MessageTable[MyTableIndex][0] != '\0') /* table is full */
          {
             DebugString.Buffer = "CRITICAL ERROR: DbgPrint Table is FULL!";
             DebugString.Length = 39;
             KdpPrintString(&DebugString);
             for (;;);
          }
        else
          {
             /*DebugString.Buffer = "µµµ";
             DebugString.Length = 3;
             KdpPrintString(&DebugString);*/
             memcpy(MessageTable[MyTableIndex], DebugString.Buffer, DebugString.Length);
             MessageTable[MyTableIndex][DebugString.Length] = '\0';
          }
     }
   else
     {
#endif /* SERIALIZE_DBGPRINT */
        KdpPrintString (&DebugString);
#ifdef SERIALIZE_DBGPRINT
        MyTableIndex = TableReadIndex;
        while (MessageTable[MyTableIndex][0] != '\0')
          {
             /*DebugString.Buffer = "$$$";
             DebugString.Length = 3;
             KdpPrintString(&DebugString);*/

             DebugString.Buffer = MessageTable[MyTableIndex];
             DebugString.Length = strlen(DebugString.Buffer);
             DebugString.MaximumLength = DebugString.Length + 1;

             KdpPrintString(&DebugString);
             MessageTable[MyTableIndex][0] = '\0';

             MyTableIndex = InterlockedIncrement(&TableReadIndex);
             InterlockedCompareExchange(&TableReadIndex, 0, MESSAGETABLE_SIZE);
             MyTableIndex %= MESSAGETABLE_SIZE;
          }
        InterlockedDecrement(&Lock);
     }
#  undef MESSAGETABLE_SIZE
#endif /* SERIALIZE_DBGPRINT */

   return (ULONG)DebugString.Length;
}

/*
 * @unimplemented
 */
ULONG
__cdecl
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    ...
    )
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
ULONG
__cdecl
DbgPrintReturnControlC(
    PCH Format,
    ...
    )
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOLEAN State
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

/* $Id: print.c,v 1.7 2000/02/13 16:05:17 dwelch Exp $
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
#include <string.h>


/* FUNCTIONS ****************************************************************/

ULONG DbgService (ULONG Service, PVOID Context1, PVOID Context2);
__asm__ ("\n\t.global _DbgService\n\t"
         "_DbgService:\n\t"
         "mov 4(%esp), %eax\n\t"
         "mov 8(%esp), %ecx\n\t"
         "mov 12(%esp), %edx\n\t"
         "int $0x2D\n\t"
         "ret\n\t");

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

   HalDisplayString(Buffer);
   
   return (ULONG)DebugString.Length;
}

/* EOF */

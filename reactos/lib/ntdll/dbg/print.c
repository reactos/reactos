/* $Id: print.c,v 1.2 2000/01/18 12:04:16 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/dbg/print.c
 * PURPOSE:         Debug output
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  Created 28/12/1999
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


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
   CHAR Buffer[512];
   va_list ap;

   /* init ansi string */
   DebugString.Buffer = Buffer;
   DebugString.MaximumLength = 512;

   va_start (ap, Format);
   DebugString.Length = _vsnprintf (Buffer, 512, Format, ap);
   va_end (ap);

   DbgService (1, &DebugString, NULL);

   return (ULONG)DebugString.Length;
}

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/mem.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/string.h>

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG RtlCompareMemory(PVOID Source1, PVOID Source2, ULONG Length)
/*
 * FUNCTION: Compares blocks of memory and returns the number of equal bytes
 * ARGUMENTS:
 *      Source1 = Block to compare
 *      Source2 = Block to compare
 *      Length = Number of bytes to compare
 * RETURNS: Number of equal bytes
 */
{
   int i,total;
   
   for (i=0,total=0;i<Length;i++)
     {
	if ( ((PUCHAR)Source1)[i] == ((PUCHAR)Source2)[i] )
	  {
	     total++;
	  }
     }
   return(total);
}

VOID RtlCopyBytes(PVOID Destination,
		  CONST VOID* Source,
		  ULONG Length)
{
   RtlCopyMemory(Destination,Source,Length);
}

VOID RtlCopyMemory(VOID* Destination, VOID* Source, ULONG Length)
{
   memcpy(Destination,Source,Length);
}

VOID RtlFillMemory(PVOID Destination, ULONG Length, UCHAR Fill)
{
   memset(Destination,Fill,Length);
}

VOID RtlZeroMemory(PVOID Destination, ULONG Length)
{
   RtlFillMemory(Destination,Length,0);
}

VOID RtlMoveMemory(PVOID Destination,
		   CONST VOID* Source,
		   ULONG Length)
{
   memmove(Destination,Source,Length);
}

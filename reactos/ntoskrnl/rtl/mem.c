/* $Id: mem.c,v 1.6 1999/11/09 18:03:39 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/mem.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG
STDCALL
RtlCompareMemory(PVOID Source1, PVOID Source2, ULONG Length)
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

VOID RtlCopyMemory(VOID* Destination, CONST VOID* Source, ULONG Length)
{
   DPRINT("RtlCopyMemory(Destination %x Source %x Length %d\n",
	  Destination,Source,Length);
   memcpy(Destination,Source,Length);
   DPRINT("*Destination %x\n",*(PULONG)Destination);
}


VOID
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	)
{
	memset(Destination,Fill,Length);
}


VOID
STDCALL
RtlFillMemoryUlong (
        PVOID   Destination,
	ULONG	Length,
        ULONG   Fill
	)
{
   PULONG Dest  = Destination;
   ULONG  Count = Length / 4;

   while (Count > 0)
   {
       *Dest = Fill;
       Dest++;
       Count--;
   }
}


VOID
STDCALL
RtlZeroMemory (
	PVOID	Destination,
	ULONG	Length
	)
{
	RtlFillMemory (
		Destination,
		Length,
		0
		);
}


VOID
STDCALL
RtlMoveMemory (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	)
{
	memmove (
		Destination,
		Source,
		Length
		);
}


/* EOF */

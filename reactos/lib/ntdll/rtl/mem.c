/* $Id: mem.c,v 1.10 2002/09/07 15:12:40 chorns Exp $
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

#define NTOS_USER_MODE
#include <ntos.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#undef RtlCompareMemory

SIZE_T STDCALL
RtlCompareMemory(
  IN CONST VOID  *Source1,
  IN CONST VOID  *Source2,
  IN SIZE_T  Length)
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

#undef RtlCompareMemoryUlong

ULONG
STDCALL
RtlCompareMemoryUlong (
	PVOID	Source,
	ULONG	Length,
	ULONG	Value
	)
/*
 * FUNCTION: Compares a block of ULONGs with an ULONG and returns the number of equal bytes
 * ARGUMENTS:
 *      Source = Block to compare
 *      Length = Number of bytes to compare
 *      Value = Value to compare
 * RETURNS: Number of equal bytes
 */
{
	PULONG ptr = (PULONG)Source;
	ULONG  len = Length / sizeof(ULONG);
	int i;

	for (i = 0; i < len; i++)
	{
		if (*ptr != Value)
			break;
		ptr++;
	}

	return (ULONG)((PCHAR)ptr - (PCHAR)Source);
}

#if 0

#undef RtlCopyBytes
VOID RtlCopyBytes(PVOID Destination,
		  CONST VOID* Source,
		  ULONG Length)
{
   RtlCopyMemory(Destination,Source,Length);
}

#undef RtlCopyMemory

VOID RtlCopyMemory(VOID* Destination, CONST VOID* Source, ULONG Length)
{
   DPRINT("RtlCopyMemory(Destination %x Source %x Length %d\n",
	  Destination,Source,Length);
   memcpy(Destination,Source,Length);
   DPRINT("*Destination %x\n",*(PULONG)Destination);
}
#endif

#undef RtlFillMemory

VOID 
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	)
{
	memset (
		Destination,
		Fill,
		Length
		);
}

#undef RtlFillMemoryUlong

VOID
STDCALL
RtlFillMemoryUlong (
	PVOID	Destination,
	ULONG	Length,
	ULONG	Fill
	)
{
	PULONG Dest  = Destination;
	ULONG  Count = Length / sizeof(ULONG);

	while (Count > 0)
	{
		*Dest = Fill;
		Dest++;
		Count--;
	}
}

#undef RtlZeroMemory

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

#undef RtlMoveMemory

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

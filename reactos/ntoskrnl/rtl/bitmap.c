/* $Id: bitmap.c,v 1.1 2000/03/03 00:48:50 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/bitmap.c
 * PURPOSE:         Bitmap functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Eric Kohl
 */

#include <ddk/ntddk.h>


#define ALIGN(x,align)	(((x)+(align)-1) / (align))


VOID
STDCALL
RtlInitializeBitMap (
	PRTL_BITMAP	BitMapHeader,
	PULONG		BitMapBuffer,
	ULONG		SizeOfBitMap
	)
{
	BitMapHeader->SizeOfBitMap = SizeOfBitMap;
	BitMapHeader->Buffer = BitMapBuffer;
}


VOID
STDCALL
RtlClearAllBits (
	IN OUT	PRTL_BITMAP	BitMapHeader
	)
{
	memset (BitMapHeader->Buffer,
	        0x00,
	        ALIGN(BitMapHeader->SizeOfBitMap, 8));
}


VOID
STDCALL
RtlClearBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		NumberToClear
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG n;
	ULONG shift;
	PCHAR p;

	if (StartingIndex >= Size || NumberToClear == 0)
		return;

	if (StartingIndex + NumberToClear > Size)
		NumberToClear = Size - StartingIndex;

	p = (PCHAR)(BitMapHeader->Buffer + (StartingIndex / 8));
	while (NumberToClear)
	{
		/* bit shift in current byte */
		shift = StartingIndex & 7;

		/* number of bits to change in current byte */
		n = (NumberToClear > 8 - shift ) ? 8 - shift : NumberToClear;

		/* adjust byte */
		*p++ &= ~(~(0xFF << n) << shift);
		NumberToClear -= n;
		StartingIndex += n;
	}
}


VOID
STDCALL
RtlSetAllBits (
	IN OUT	PRTL_BITMAP	BitMapHeader
	)
{
	memset (BitMapHeader->Buffer,
	        0xFF,
	        ALIGN(BitMapHeader->SizeOfBitMap, 8));
}

VOID
STDCALL
RtlSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		NumberToSet
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG n;
	ULONG shift;
	PCHAR p;

	if (StartingIndex >= Size || NumberToSet == 0)
		return;

	if (StartingIndex + NumberToSet > Size)
		NumberToSet = Size - StartingIndex;

	p = (PCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (NumberToSet)
	{
		/* bit shift in current byte */
		shift = StartingIndex & 7;

		/* number of bits to change in current byte */
		n = (NumberToSet > 8 - shift) ? 8 - shift : NumberToSet;

		/* adjust byte */
		*p++ |= ~( 0xFF << n ) << shift;
		NumberToSet -= n;
		StartingIndex += n;
	}
}


/* EOF */

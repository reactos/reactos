/* $Id: bitmap.c,v 1.7 2003/01/19 01:49:10 hbirr Exp $
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

#define MASK(Count, Shift) ((Count) == 32 ? 0xFFFFFFFF : ~(0xFFFFFFFF << (Count)) << (Shift))


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


BOOLEAN
STDCALL
RtlAreBitsClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Shift;
	ULONG Count;
	PULONG Ptr;

	if (StartingIndex >= Size ||
	    !Length ||
	    (StartingIndex + Length > Size))
		return FALSE;

	Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
	while (Length)
	{
		/* get bit shift in current dword */
		Shift = StartingIndex & 0x1F;

		/* get number of bits to check in current dword */
		Count = (Length > 32 - Shift) ? 32 - Shift : Length;

		/* check dword */
		if (*Ptr++ & MASK(Count, Shift))
			return FALSE;

		Length -= Count;
		StartingIndex += Count;
	}

	return TRUE;
}


BOOLEAN
STDCALL
RtlAreBitsSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Shift;
	ULONG Count;
	PULONG Ptr;

	if (StartingIndex >= Size ||
	    !Length ||
	    (StartingIndex + Length > Size))
		return FALSE;

	Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
	while (Length)
	{
		/* get bit shift in current dword */
		Shift = StartingIndex & 0x1F;

		/* get number of bits to check in current dword */
		Count = (Length > 32 - Shift) ? 32 - Shift : Length;

		/* check dword */
		if (~*Ptr++ & MASK(Count, Shift))
			return FALSE;

		Length -= Count;
		StartingIndex += Count;
	}

	return TRUE;
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
	ULONG Count;
	ULONG Shift;
	PULONG Ptr;

	if (StartingIndex >= Size || NumberToClear == 0)
		return;

	if (StartingIndex + NumberToClear > Size)
		NumberToClear = Size - StartingIndex;

	Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
	while (NumberToClear)
	{
		/* bit shift in current dword */
		Shift = StartingIndex & 0x1F;

		/* number of bits to change in current dword */
		Count = (NumberToClear > 32 - Shift ) ? 32 - Shift : NumberToClear;

		/* adjust dword */
		*Ptr++ &= ~MASK(Count, Shift);
		NumberToClear -= Count;
		StartingIndex += Count;
	}
}


ULONG
STDCALL
RtlFindClearBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PULONG Ptr;
	ULONG  Mask;

	if (NumberToFind > Size || NumberToFind == 0)
		return -1;

	if (HintIndex >= Size)
		HintIndex = 0;

	Index = HintIndex;
	Ptr = (PULONG)BitMapHeader->Buffer + (Index / 32);
	Mask  = 1 << (Index & 0x1F);

	while (HintIndex < Size)
	{
		/* count clear bits */
		for (Count = 0; Index < Size && ~*Ptr & Mask; Index++)
		{
			if (++Count >= NumberToFind)
				return HintIndex;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip set bits */
		for (; Index < Size && *Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}
		HintIndex = Index;
	}

	return -1;
}


ULONG
STDCALL
RtlFindClearBitsAndSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Index;

	Index = RtlFindClearBits (BitMapHeader,
	                          NumberToFind,
	                          HintIndex);
	if (Index != (ULONG)-1)
		RtlSetBits (BitMapHeader,
		            Index,
		            NumberToFind);

	return Index;
}


ULONG
STDCALL
RtlFindFirstRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PULONG Ptr;
	ULONG  Mask;

	if (*StartingIndex > Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}

	Index = *StartingIndex;
	Ptr = (PULONG)BitMapHeader->Buffer + (Index / 32);
	Mask = 1 << (Index & 0x1F);

	/* skip set bits */
	for (; Index < Size && *Ptr & Mask; Index++)
	{
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	/* return index of first clear bit */
	if (Index >= Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}
	else
		*StartingIndex = Index;

	/* count clear bits */
	for (Count = 0; Index < Size && ~*Ptr & Mask; Index++)
	{
		Count++;
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


ULONG
STDCALL
RtlFindFirstRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PULONG Ptr;
	ULONG  Mask;

	if (*StartingIndex > Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}

	Index = *StartingIndex;
	Ptr = (PULONG)BitMapHeader->Buffer + (Index / 32);
	Mask = 1 << (Index & 0x1F);

	/* skip clear bits */
	for (; Index < Size && ~*Ptr & Mask; Index++)
	{
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	/* return index of first set bit */
	if (Index >= Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}
	else
		*StartingIndex = Index;

	/* count set bits */
	for (Count = 0; Index < Size && *Ptr & Mask; Index++)
	{
		Count++;
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


ULONG
STDCALL
RtlFindLongestRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	PULONG Ptr = (PULONG)BitMapHeader->Buffer;
	ULONG Index = 0;
	ULONG Count;
	ULONG Max = 0;
	ULONG Start;
	ULONG Maxstart = 0;
	ULONG  Mask = 1;

	while (Index < Size)
	{
		Start = Index;

		/* count clear bits */
		for (Count = 0; Index < Size && ~*Ptr & Mask; Index++)
		{
			Count++;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip set bits */
		for (; Index < Size && *Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		if (Count > Max)
		{
			Max = Count;
			Maxstart = Start;
		}
	}

	if (StartingIndex)
		*StartingIndex = Maxstart;

	return Max;
}


ULONG
STDCALL
RtlFindLongestRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	PULONG Ptr = (PULONG)BitMapHeader->Buffer;
	ULONG Index = 0;
	ULONG Count;
	ULONG Max = 0;
	ULONG Start;
	ULONG Maxstart = 0;
	ULONG  Mask = 1;

	while (Index < Size)
	{
		Start = Index;

		/* count set bits */
		for (Count = 0; Index < Size && *Ptr & Mask; Index++)
		{
			Count++;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip clear bits */
		for (; Index < Size && ~*Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		if (Count > Max)
		{
			Max = Count;
			Maxstart = Start;
		}
	}

	if (StartingIndex)
		*StartingIndex = Maxstart;

	return Max;
}


ULONG
STDCALL
RtlFindSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PULONG Ptr;
	CHAR  Mask;

	if (NumberToFind > Size || NumberToFind == 0)
		return (ULONG)-1;

	if (HintIndex >= Size)
		HintIndex = 0;

	Index = HintIndex;
	Ptr = (PULONG)BitMapHeader->Buffer + (Index / 32);
	Mask = 1 << (Index & 0x1F);

	while (HintIndex < Size)
	{
		/* count set bits */
		for (Count = 0; Index < Size && *Ptr & Mask; Index++)
		{
			if (++Count >= NumberToFind)
				return HintIndex;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip clear bits */
		for (; Index < Size && ~*Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}
		HintIndex = Index;
	}

	return (ULONG)-1;
}


ULONG
STDCALL
RtlFindSetBitsAndClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Index;

	Index = RtlFindSetBits (BitMapHeader,
	                        NumberToFind,
	                        HintIndex);
	if (Index != (ULONG)-1)
		RtlClearBits (BitMapHeader,
		              Index,
		              NumberToFind);

	return Index;
}


ULONG
STDCALL
RtlNumberOfClearBits (
	PRTL_BITMAP	BitMapHeader
	)
{
	PULONG Ptr = (PULONG)BitMapHeader->Buffer;
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	ULONG Mask;

	for (Mask = 1, Index = 0, Count = 0; Index < Size; Index++)
	{
		if (~*Ptr & Mask)
			Count++;

		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


ULONG
STDCALL
RtlNumberOfSetBits (
	PRTL_BITMAP	BitMapHeader
	)
{
	PULONG Ptr = (PULONG)BitMapHeader->Buffer;
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	ULONG Mask;

	for (Mask = 1, Index = 0, Count = 0; Index < Size; Index++)
	{
		if (*Ptr & Mask)
			Count++;

		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
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
	ULONG Count;
	ULONG Shift;
	PULONG Ptr;

	if (StartingIndex >= Size || NumberToSet == 0)
		return;

	if (StartingIndex + NumberToSet > Size)
		NumberToSet = Size - StartingIndex;

	Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
	while (NumberToSet)
	{
		/* bit shift in current dword */
		Shift = StartingIndex & 0x1F;

		/* number of bits to change in current dword */
		Count = (NumberToSet > 32 - Shift) ? 32 - Shift : NumberToSet;

		/* adjust dword */
		*Ptr++ |= MASK(Count, Shift);
		NumberToSet -= Count;
		StartingIndex += Count;
	}
}

/* EOF */

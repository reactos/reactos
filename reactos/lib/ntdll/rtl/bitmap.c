/* $Id: bitmap.c,v 1.5 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/bitmap.c
 * PURPOSE:         Bitmap functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Eric Kohl
 */

#include <ddk/ntddk.h>


#define NDEBUG
#include <ntdll/ntdll.h>

#define ALIGN(x,align)	(((x)+(align)-1) / (align))


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
	PUCHAR Ptr;

	if (StartingIndex >= Size ||
	    !Length ||
	    (StartingIndex + Length > Size))
		return FALSE;

	Ptr = (PUCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (Length)
	{
		/* get bit shift in current byte */
		Shift = StartingIndex & 7;

		/* get number of bits to check in current byte */
		Count = (Length > 8 - Shift) ? 8 - Shift : Length;

		/* check byte */
		if (*Ptr & (~(0xFF << Count) << Shift))
			return FALSE;

		Ptr++;
		Length -= Count;
		StartingIndex += Count;
	}

	return TRUE;
}


/*
 * @implemented
 */
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
	PUCHAR Ptr;
	UCHAR Check;

	if (StartingIndex >= Size ||
	    !Length ||
	    (StartingIndex + Length > Size))
		return FALSE;

	Ptr = (PUCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (Length)
	{
		/* get bit shift in current byte */
		Shift = StartingIndex & 7;

		/* get number of bits to check in current byte */
		Count = (Length > 8 - Shift) ? 8 - Shift : Length;

		/* bulid check byte */
		Check = ~(0xFF << Count) << Shift;

		/* check byte */
		if ((*Ptr & Check) != Check)
			return FALSE;

		Ptr++;
		Length -= Count;
		StartingIndex += Count;
	}

	return TRUE;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
	PCHAR Ptr;

	if (StartingIndex >= Size || NumberToClear == 0)
		return;

	if (StartingIndex + NumberToClear > Size)
		NumberToClear = Size - StartingIndex;

	Ptr = (PCHAR)(BitMapHeader->Buffer + (StartingIndex / 8));
	while (NumberToClear)
	{
		/* bit shift in current byte */
		Shift = StartingIndex & 7;

		/* number of bits to change in current byte */
		Count = (NumberToClear > 8 - Shift ) ? 8 - Shift : NumberToClear;

		/* adjust byte */
		*Ptr &= ~(~(0xFF << Count) << Shift);

		Ptr++;
		NumberToClear -= Count;
		StartingIndex += Count;
	}
}


/*
 * @implemented
 */
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
	PCHAR Ptr;
	CHAR  Mask;

	if (NumberToFind > Size || NumberToFind == 0)
		return -1;

	if (HintIndex >= Size)
		HintIndex = 0;

	Index = HintIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask  = 1 << (Index & 7);

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


/*
 * @implemented
 */
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
	PCHAR Ptr;
	CHAR  Mask;

	if (*StartingIndex > Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}

	Index = *StartingIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask = 1 << (Index & 7);

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
	PCHAR Ptr;
	CHAR  Mask;

	if (*StartingIndex > Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}

	Index = *StartingIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask = 1 << (Index & 7);

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


/*
 * @implemented
 */
ULONG
STDCALL
RtlFindLongestRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Index = 0;
	ULONG Count;
	ULONG Max = 0;
	ULONG Start;
	ULONG Maxstart = 0;
	CHAR  Mask = 1;

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


/*
 * @implemented
 */
ULONG
STDCALL
RtlFindLongestRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Index = 0;
	ULONG Count;
	ULONG Max = 0;
	ULONG Start;
	ULONG Maxstart = 0;
	CHAR  Mask = 1;

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


/*
 * @implemented
 */
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
	PCHAR Ptr;
	CHAR  Mask;

	if (NumberToFind > Size || NumberToFind == 0)
		return (ULONG)-1;

	if (HintIndex >= Size)
		HintIndex = 0;

	Index = HintIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask = 1 << (Index & 7);

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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
ULONG
STDCALL
RtlNumberOfClearBits (
	PRTL_BITMAP	BitMapHeader
	)
{
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	CHAR Mask;

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


/*
 * @implemented
 */
ULONG
STDCALL
RtlNumberOfSetBits (
	PRTL_BITMAP	BitMapHeader
	)
{
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	CHAR Mask;

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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
	PCHAR Ptr;

	if (StartingIndex >= Size || NumberToSet == 0)
		return;

	if (StartingIndex + NumberToSet > Size)
		NumberToSet = Size - StartingIndex;

	Ptr = (PCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (NumberToSet)
	{
		/* bit shift in current byte */
		Shift = StartingIndex & 7;

		/* number of bits to change in current byte */
		Count = (NumberToSet > 8 - Shift) ? 8 - Shift : NumberToSet;

		/* adjust byte */
		*Ptr |= ~(0xFF << Count) << Shift;

		Ptr++;
		NumberToSet -= Count;
		StartingIndex += Count;
	}
}

/* EOF */

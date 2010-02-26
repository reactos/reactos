/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/bitmap.c
 * PURPOSE:         Bitmap functions
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

/* Number of set bits per byte value */
static const
UCHAR
BitCountTable[256] =
{
    /* x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
       0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, /* 0x */
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 1x */
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 2x */
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 3x */
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 4x */
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 5x */
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 6c */
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* 7x */
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 8x */
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 9x */
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* Ax */
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* Bx */
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* Cx */
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* Dx */
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* Ex */
       4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8  /* Fx */
};


/* PRIVATE FUNCTIONS ********************************************************/

static __inline__
ULONG
RtlpGetLengthOfRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG MaxLength)
{
    ULONG Value, BitPos, Length;
    PULONG Buffer, MaxBuffer;

    /* Calculate positions */
    Buffer = BitMapHeader->Buffer + StartingIndex / 32;
    BitPos = StartingIndex & 31;

    /* Calculate the maximum length */
    MaxLength = min(MaxLength, BitMapHeader->SizeOfBitMap - StartingIndex);
    MaxBuffer = Buffer + (BitPos + MaxLength + 31) / 32;

    /* Clear the bits that don't belong to this run */
    Value = *Buffer++ >> BitPos << BitPos;

    /* Skip all clear ULONGs */
    while (Value == 0 && Buffer < MaxBuffer)
    {
        Value = *Buffer++;
    }

    /* Did we reach the end? */
    if (Value == 0)
    {
        /* Return maximum length */
        return MaxLength;
    }

    /* We hit a set bit, check how many clear bits are left */
    BitScanForward(&BitPos, Value);

    /* Calculate length up to where we read */
    Length = (Buffer - BitMapHeader->Buffer) * 32 - StartingIndex;
    Length += BitPos - 32;

    /* Make sure we don't go past the last bit */
    if (Length > BitMapHeader->SizeOfBitMap - StartingIndex)
        Length = BitMapHeader->SizeOfBitMap - StartingIndex;

    /* Return the result */
    return Length;
}

static __inline__
ULONG
RtlpGetLengthOfRunSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG MaxLength)
{
    ULONG InvValue, BitPos, Length;
    PULONG Buffer, MaxBuffer;

    /* Calculate positions */
    Buffer = BitMapHeader->Buffer + StartingIndex / 32;
    BitPos = StartingIndex & 31;

    /* Calculate the maximum length */
    MaxLength = min(MaxLength, BitMapHeader->SizeOfBitMap - StartingIndex);
    MaxBuffer = Buffer + (BitPos + MaxLength + 31) / 32;

    /* Get the inversed value, clear bits that don't belong to the run */
    InvValue = ~(*Buffer++) >> BitPos << BitPos;

    /* Skip all set ULONGs */
    while (InvValue == 0 && Buffer < MaxBuffer)
    {
        InvValue = ~(*Buffer++);
    }

    /* Did we reach the end? */
    if (InvValue == 0)
    {
        /* Yes, return maximum */
        return MaxLength;
    }

    /* We hit a clear bit, check how many set bits are left */
    BitScanForward(&BitPos, InvValue);

    /* Calculate length up to where we read */
    Length = (Buffer - BitMapHeader->Buffer) * 32 - StartingIndex;
    Length += BitPos - 32;

    /* Make sure we don't go past the last bit */
    if (Length > BitMapHeader->SizeOfBitMap - StartingIndex)
        Length = BitMapHeader->SizeOfBitMap - StartingIndex;

    /* Return the result */
    return Length;
}


/* PUBLIC FUNCTIONS **********************************************************/

CCHAR
NTAPI
RtlFindMostSignificantBit(ULONGLONG Value)
{
    ULONG Position;

    if (BitScanReverse(&Position, Value >> 32))
    {
        return Position + 32;
    }
    else if (BitScanReverse(&Position, (ULONG)Value))
    {
        return Position;
    }

    return -1;
}

CCHAR
NTAPI
RtlFindLeastSignificantBit(ULONGLONG Value)
{
    ULONG Position;

    if (BitScanForward(&Position, (ULONG)Value))
    {
        return Position;
    }
    else if (BitScanForward(&Position, Value >> 32))
    {
        return Position + 32;
    }

    return -1;
}

VOID
NTAPI
RtlInitializeBitMap(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG BitMapBuffer,
    IN ULONG SizeOfBitMap)
{
    // FIXME: some bugger here!
    //ASSERT(SizeOfBitMap > 0);

    /* Setup the bitmap header */
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer = BitMapBuffer;
}

VOID
NTAPI
RtlClearAllBits(
    IN OUT PRTL_BITMAP BitMapHeader)
{
    ULONG LengthInUlongs;

    LengthInUlongs = (BitMapHeader->SizeOfBitMap + 31) >> 5;
    RtlFillMemoryUlong(BitMapHeader->Buffer, LengthInUlongs << 2, 0);
}

VOID
NTAPI
RtlSetAllBits(
    IN OUT PRTL_BITMAP BitMapHeader)
{
    ULONG LengthInUlongs;
    
    LengthInUlongs = (BitMapHeader->SizeOfBitMap + 31) >> 5;
    RtlFillMemoryUlong(BitMapHeader->Buffer, LengthInUlongs << 2, ~0);
}

VOID
NTAPI
RtlClearBit(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG BitNumber)
{
    ASSERT(BitNumber <= BitMapHeader->SizeOfBitMap);
    BitMapHeader->Buffer[BitNumber >> 5] &= ~(1 << (BitNumber & 31));
}

VOID
NTAPI
RtlSetBit(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG BitNumber)
{
    ASSERT(BitNumber <= BitMapHeader->SizeOfBitMap);
    BitMapHeader->Buffer[BitNumber >> 5] |= (1 << (BitNumber & 31));
}

VOID
NTAPI
RtlClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToClear)
{
    ULONG Bits, Mask;
    PULONG Buffer;

    ASSERT(StartingIndex + NumberToClear <= BitMapHeader->SizeOfBitMap);

    /* Calculate buffer start and first bit index */
    Buffer = &BitMapHeader->Buffer[StartingIndex >> 5];
    Bits = StartingIndex & 31;

    /* Are we unaligned? */
    if (Bits)
    {
        /* Create an inverse mask by shifting MAXULONG */
        Mask = MAXULONG << Bits;

        /* This is what's left in the first ULONG */
        Bits = 32 - Bits;

        /* Even less bits to clear? */
        if (NumberToClear < Bits)
        {
            /* Calculate how many bits are left */
            Bits -= NumberToClear;

            /* Fixup the mask on the high side */
            Mask = Mask << Bits >> Bits;

            /* Clear bits and return */
            *Buffer &= ~Mask;
            return;
        }

        /* Clear bits */
        *Buffer &= ~Mask;

        /* Update buffer and left bits */
        Buffer++;
        NumberToClear -= Bits;
    }

    /* Clear all full ULONGs */
    RtlFillMemoryUlong(Buffer, NumberToClear >> 3, 0);
    Buffer += NumberToClear >> 5;

    /* Clear what's left */
    NumberToClear &= 31;
    Mask = MAXULONG << NumberToClear;
    *Buffer &= Mask;
}

VOID
NTAPI
RtlSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToSet)
{
    ULONG Bits, Mask;
    PULONG Buffer;

    ASSERT(StartingIndex + NumberToSet <= BitMapHeader->SizeOfBitMap);

    /* Calculate buffer start and first bit index */
    Buffer = &BitMapHeader->Buffer[StartingIndex >> 5];
    Bits = StartingIndex & 31;

    /* Are we unaligned? */
    if (Bits)
    {
        /* Create a mask by shifting MAXULONG */
        Mask = MAXULONG << Bits;

        /* This is what's left in the first ULONG */
        Bits = 32 - Bits;

        /* Even less bits to clear? */
        if (NumberToSet < Bits)
        {
            /* Calculate how many bits are left */
            Bits -= NumberToSet;

            /* Fixup the mask on the high side */
            Mask = Mask << Bits >> Bits;

            /* Set bits and return */
            *Buffer |= Mask;
            return;
        }

        /* Set bits */
        *Buffer |= Mask;

        /* Update buffer and left bits */
        Buffer++;
        NumberToSet -= Bits;
    }

    /* Set all full ULONGs */
    RtlFillMemoryUlong(Buffer, NumberToSet >> 3, MAXULONG);
    Buffer += NumberToSet >> 5;

    /* Set what's left */
    NumberToSet &= 31;
    Mask = MAXULONG << NumberToSet;
    *Buffer |= ~Mask;
}

BOOLEAN
NTAPI
RtlTestBit(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG BitNumber)
{
    ASSERT(BitNumber < BitMapHeader->SizeOfBitMap);
    return (BitMapHeader->Buffer[BitNumber >> 5] >> (BitNumber & 31)) & 1;
}

BOOLEAN
NTAPI
RtlAreBitsClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length)
{
    return RtlpGetLengthOfRunClear(BitMapHeader, StartingIndex, Length) >= Length;
}

BOOLEAN
NTAPI
RtlAreBitsSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length)
{
    return RtlpGetLengthOfRunSet(BitMapHeader, StartingIndex, Length) >= Length;
}

ULONG
NTAPI
RtlNumberOfSetBits(
    IN PRTL_BITMAP BitMapHeader)
{
    PUCHAR Byte, MaxByte;
    ULONG BitCount = 0;

    Byte = (PUCHAR)BitMapHeader->Buffer;
    MaxByte = Byte + (BitMapHeader->SizeOfBitMap + 7) / 8;

    do
    {
        BitCount += BitCountTable[*Byte++];
    }
    while (Byte <= MaxByte);

    return BitCount;
}

ULONG
NTAPI
RtlNumberOfClearBits(
    IN PRTL_BITMAP BitMapHeader)
{
    /* Do some math */
    return BitMapHeader->SizeOfBitMap - RtlNumberOfSetBits(BitMapHeader);
}

ULONG
NTAPI
RtlFindClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex)
{
    ULONG CurrentBit, Margin, CurrentLength;

    /* Check for valid parameters */
    if (!BitMapHeader || NumberToFind > BitMapHeader->SizeOfBitMap)
    {
        return MAXULONG;
    }

    /* Check for trivial case */
    if (NumberToFind == 0)
    {
        return HintIndex;
    }

    /* First margin is end of bitmap */
    Margin = BitMapHeader->SizeOfBitMap;

retry:
    /* Start with hint index, length is 0 */
    CurrentBit = HintIndex;

    /* Loop until something is found or the end is reached */
    while (CurrentBit + NumberToFind < Margin)
    {
        /* Search for the next clear run, by skipping a set run */
        CurrentBit += RtlpGetLengthOfRunSet(BitMapHeader,
                                            CurrentBit,
                                            MAXULONG);

        /* Get length of the clear bit run */
        CurrentLength = RtlpGetLengthOfRunClear(BitMapHeader,
                                                CurrentBit,
                                                NumberToFind);

        /* Is this long enough? */
        if (CurrentLength >= NumberToFind)
        {
            /* It is */
            return CurrentBit;
        }

        CurrentBit += CurrentLength;
    }

    /* Did we start at a hint? */
    if (HintIndex)
    {
        /* Retry at the start */
        Margin = min(HintIndex + NumberToFind, BitMapHeader->SizeOfBitMap);
        HintIndex = 0;
        goto retry;
    }

    /* Nothing found */
    return MAXULONG;
}

ULONG
NTAPI
RtlFindSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex)
{
    ULONG CurrentBit, CurrentLength;

    /* Check for valid parameters */
    if (!BitMapHeader || NumberToFind > BitMapHeader->SizeOfBitMap)
    {
        return MAXULONG;
    }

    /* Check for trivial case */
    if (NumberToFind == 0)
    {
        return HintIndex;
    }

    /* Start with hint index, length is 0 */
    CurrentBit = HintIndex;
    CurrentLength = 0;

    /* Loop until something is found or the end is reached */
    while (CurrentBit + NumberToFind <= BitMapHeader->SizeOfBitMap)
    {
        /* Search for the next set run, by skipping a clear run */
        CurrentBit += RtlpGetLengthOfRunClear(BitMapHeader,
                                              CurrentBit,
                                              MAXULONG);

        /* Get length of the set bit run */
        CurrentLength = RtlpGetLengthOfRunSet(BitMapHeader,
                                              CurrentBit,
                                              NumberToFind);

        /* Is this long enough? */
        if (CurrentLength >= NumberToFind)
        {
            /* It is */
            return CurrentBit;
        }

        CurrentBit += CurrentLength;
    }

    /* Nothing found */
    return MAXULONG;
}

ULONG
NTAPI
RtlFindClearBitsAndSet(
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex)
{
    ULONG Position;

    /* Try to find clear bits */
    Position = RtlFindClearBits(BitMapHeader, NumberToFind, HintIndex);

    /* Did we get something? */
    if (Position != MAXULONG)
    {
        /* Yes, set the bits */
        RtlSetBits(BitMapHeader, Position, NumberToFind);
    }

    /* Return what we found */
    return Position;
}

ULONG
NTAPI
RtlFindSetBitsAndClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex)
{
    ULONG Position;

    /* Try to find set bits */
    Position = RtlFindSetBits(BitMapHeader, NumberToFind, HintIndex);

    /* Did we get something? */
    if (Position != MAXULONG)
    {
        /* Yes, clear the bits */
        RtlClearBits(BitMapHeader, Position, NumberToFind);
    }

    /* Return what we found */
    return Position;
}

ULONG
NTAPI
RtlFindNextForwardRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex)
{
    ULONG Length;

    /* Assume a set run first, count it's length */
    Length = RtlpGetLengthOfRunSet(BitMapHeader, FromIndex, MAXULONG);
    *StartingRunIndex = FromIndex + Length;

    /* Now return the length of the run */
    return RtlpGetLengthOfRunClear(BitMapHeader, FromIndex + Length, MAXULONG);
}

ULONG
NTAPI
RtlFindNextForwardRunSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex)
{
    ULONG Length;

    /* Assume a clear run first, count it's length */
    Length = RtlpGetLengthOfRunClear(BitMapHeader, FromIndex, MAXULONG);
    *StartingRunIndex = FromIndex + Length;

    /* Now return the length of the run */
    return RtlpGetLengthOfRunSet(BitMapHeader, FromIndex, MAXULONG);
}

ULONG
NTAPI
RtlFindFirstRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG StartingIndex)
{
    return RtlFindNextForwardRunClear(BitMapHeader, 0, StartingIndex);
}

ULONG
NTAPI
RtlFindLastBackwardRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex)
{
    UNIMPLEMENTED;
    return 0;
}


ULONG
NTAPI
RtlFindClearRuns(
    IN PRTL_BITMAP BitMapHeader,
    IN PRTL_BITMAP_RUN RunArray,
    IN ULONG SizeOfRunArray,
    IN BOOLEAN LocateLongestRuns)
{
    ULONG StartingIndex, NumberOfBits, Run, FromIndex = 0, SmallestRun = 0;

    /* Loop the runs */
    for (Run = 0; Run < SizeOfRunArray; Run++)
    {
        /* Look for a run */
        NumberOfBits = RtlFindNextForwardRunClear(BitMapHeader, 
                                                  FromIndex,
                                                  &StartingIndex);

        /* Nothing more found? Quit looping. */
        if (NumberOfBits == 0) break;

        /* Add another run */
        RunArray[Run].StartingIndex = StartingIndex;
        RunArray[Run].NumberOfBits = NumberOfBits;

        /* Update smallest run */
        if (NumberOfBits < RunArray[SmallestRun].NumberOfBits)
        {
            SmallestRun = Run;
        }

        /* Advance bits */
        FromIndex = StartingIndex + NumberOfBits;
    }

    /* Check if we are finished */
    if (Run < SizeOfRunArray || !LocateLongestRuns)
    {
        /* Return the number of found runs */
        return Run;
    }

    while (1)
    {
        /* Look for a run */
        NumberOfBits = RtlFindNextForwardRunClear(BitMapHeader, 
                                                  FromIndex,
                                                  &StartingIndex);

        /* Nothing more found? Quit looping. */
        if (NumberOfBits == 0) break;

        /* Check if we have something to update */
        if (NumberOfBits > RunArray[SmallestRun].NumberOfBits)
        {
            /* Update smallest run */
            RunArray[SmallestRun].StartingIndex = StartingIndex;
            RunArray[SmallestRun].NumberOfBits = NumberOfBits;

            /* Loop all runs */
            for (Run = 0; Run < SizeOfRunArray; Run++)
            {
                /*Is this the new smallest run? */
                if (NumberOfBits < RunArray[SmallestRun].NumberOfBits)
                {
                    /* Set it as new smallest run */
                    SmallestRun = Run;
                }
            }
        }

        /* Advance bits */
        FromIndex += NumberOfBits;
    }

    return Run;
}

ULONG
NTAPI
RtlFindLongestRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG StartingIndex)
{
    ULONG NumberOfBits, Index, MaxNumberOfBits = 0, FromIndex = 0;

    while (1)
    {
        /* Look for a run */
        NumberOfBits = RtlFindNextForwardRunClear(BitMapHeader, 
                                                  FromIndex,
                                                  &Index);

        /* Nothing more found? Quit looping. */
        if (NumberOfBits == 0) break;

        /* Was that the longest run? */
        if (NumberOfBits > MaxNumberOfBits)
        {
            /* Update values */
            MaxNumberOfBits = NumberOfBits;
            *StartingIndex = Index;
        }

        /* Advance bits */
        FromIndex += NumberOfBits;
    }

    return MaxNumberOfBits;
}

ULONG
NTAPI
RtlFindLongestRunSet(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG StartingIndex)
{
    ULONG NumberOfBits, Index, MaxNumberOfBits = 0, FromIndex = 0;

    while (1)
    {
        /* Look for a run */
        NumberOfBits = RtlFindNextForwardRunSet(BitMapHeader, 
                                                FromIndex,
                                                &Index);

        /* Nothing more found? Quit looping. */
        if (NumberOfBits == 0) break;

        /* Was that the longest run? */
        if (NumberOfBits > MaxNumberOfBits)
        {
            /* Update values */
            MaxNumberOfBits = NumberOfBits;
            *StartingIndex = Index;
        }

        /* Advance bits */
        FromIndex += NumberOfBits;
    }

    return MaxNumberOfBits;
}


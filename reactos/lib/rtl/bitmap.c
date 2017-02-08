/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 *                  BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/rtl/bitmap.c
 * PURPOSE:         Bitmap functions
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

// FIXME: hack
#undef ASSERT
#define ASSERT(...)

#ifdef USE_RTL_BITMAP64
#define _BITCOUNT 64
#define MAXINDEX 0xFFFFFFFFFFFFFFFF
typedef ULONG64 BITMAP_INDEX, *PBITMAP_INDEX;
typedef ULONG64 BITMAP_BUFFER, *PBITMAP_BUFFER;
#define RTL_BITMAP RTL_BITMAP64
#define PRTL_BITMAP PRTL_BITMAP64
#define RTL_BITMAP_RUN RTL_BITMAP_RUN64
#define PRTL_BITMAP_RUN PRTL_BITMAP_RUN64
#undef BitScanForward
#define BitScanForward(Index, Mask) \
    do { unsigned long tmp; BitScanForward64(&tmp, Mask); *Index = tmp; } while (0)
#undef BitScanReverse
#define BitScanReverse(Index, Mask) \
    do { unsigned long tmp; BitScanReverse64(&tmp, Mask); *Index = tmp; } while (0)
#define RtlFillMemoryUlong RtlFillMemoryUlonglong

#define RtlInitializeBitMap RtlInitializeBitMap64
#define RtlClearAllBits RtlClearAllBits64
#define RtlSetAllBits RtlSetAllBits64
#define RtlClearBit RtlClearBit64
#define RtlSetBit RtlSetBit64
#define RtlClearBits RtlClearBits64
#define RtlSetBits RtlSetBits64
#define RtlTestBit RtlTestBit64
#define RtlAreBitsClear RtlAreBitsClear64
#define RtlAreBitsSet RtlAreBitsSet64
#define RtlNumberOfSetBits RtlNumberOfSetBits64
#define RtlNumberOfClearBits RtlNumberOfClearBits64
#define RtlFindClearBits RtlFindClearBits64
#define RtlFindSetBits RtlFindSetBits64
#define RtlFindClearBitsAndSet RtlFindClearBitsAndSet64
#define RtlFindSetBitsAndClear RtlFindSetBitsAndClear64
#define RtlFindNextForwardRunClear RtlFindNextForwardRunClear64
#define RtlFindNextForwardRunSet RtlFindNextForwardRunSet64
#define RtlFindFirstRunClear RtlFindFirstRunClear64
#define RtlFindLastBackwardRunClear RtlFindLastBackwardRunClear64
#define RtlFindClearRuns RtlFindClearRuns64
#define RtlFindLongestRunClear RtlFindLongestRunClear64
#define RtlFindLongestRunSet RtlFindLongestRunSet64
#else
#define _BITCOUNT 32
#define MAXINDEX 0xFFFFFFFF
typedef ULONG BITMAP_INDEX, *PBITMAP_INDEX;
typedef ULONG BITMAP_BUFFER, *PBITMAP_BUFFER;
#endif

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

static __inline
BITMAP_INDEX
RtlpGetLengthOfRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX StartingIndex,
    _In_ BITMAP_INDEX MaxLength)
{
    BITMAP_INDEX Value, BitPos, Length;
    PBITMAP_BUFFER Buffer, MaxBuffer;

    /* If we are already at the end, the length of the run is zero */
    ASSERT(StartingIndex <= BitMapHeader->SizeOfBitMap);
    if (StartingIndex >= BitMapHeader->SizeOfBitMap)
        return 0;

    /* Calculate positions */
    Buffer = BitMapHeader->Buffer + StartingIndex / _BITCOUNT;
    BitPos = StartingIndex & (_BITCOUNT - 1);

    /* Calculate the maximum length */
    MaxLength = min(MaxLength, BitMapHeader->SizeOfBitMap - StartingIndex);
    MaxBuffer = Buffer + (BitPos + MaxLength + _BITCOUNT - 1) / _BITCOUNT;

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
    Length = (BITMAP_INDEX)(Buffer - BitMapHeader->Buffer) * _BITCOUNT - StartingIndex;
    Length += BitPos - _BITCOUNT;

    /* Make sure we don't go past the last bit */
    if (Length > BitMapHeader->SizeOfBitMap - StartingIndex)
        Length = BitMapHeader->SizeOfBitMap - StartingIndex;

    /* Return the result */
    return Length;
}

static __inline
BITMAP_INDEX
RtlpGetLengthOfRunSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX StartingIndex,
    _In_ BITMAP_INDEX MaxLength)
{
    BITMAP_INDEX InvValue, BitPos, Length;
    PBITMAP_BUFFER Buffer, MaxBuffer;

    /* If we are already at the end, the length of the run is zero */
    ASSERT(StartingIndex <= BitMapHeader->SizeOfBitMap);
    if (StartingIndex >= BitMapHeader->SizeOfBitMap)
        return 0;

    /* Calculate positions */
    Buffer = BitMapHeader->Buffer + StartingIndex / _BITCOUNT;
    BitPos = StartingIndex & (_BITCOUNT - 1);

    /* Calculate the maximum length */
    MaxLength = min(MaxLength, BitMapHeader->SizeOfBitMap - StartingIndex);
    MaxBuffer = Buffer + (BitPos + MaxLength + _BITCOUNT - 1) / _BITCOUNT;

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
    Length = (ULONG)(Buffer - BitMapHeader->Buffer) * _BITCOUNT - StartingIndex;
    Length += BitPos - _BITCOUNT;

    /* Make sure we don't go past the last bit */
    if (Length > BitMapHeader->SizeOfBitMap - StartingIndex)
        Length = BitMapHeader->SizeOfBitMap - StartingIndex;

    /* Return the result */
    return Length;
}


/* PUBLIC FUNCTIONS **********************************************************/

#ifndef USE_RTL_BITMAP64
CCHAR
NTAPI
RtlFindMostSignificantBit(ULONGLONG Value)
{
    ULONG Position;

#ifdef _M_AMD64
    if (BitScanReverse64(&Position, Value))
    {
        return (CCHAR)Position;
    }
#else
    if (BitScanReverse(&Position, Value >> _BITCOUNT))
    {
        return (CCHAR)(Position + _BITCOUNT);
    }
    else if (BitScanReverse(&Position, (ULONG)Value))
    {
        return (CCHAR)Position;
    }
#endif
    return -1;
}

CCHAR
NTAPI
RtlFindLeastSignificantBit(ULONGLONG Value)
{
    ULONG Position;

#ifdef _M_AMD64
    if (BitScanForward64(&Position, Value))
    {
        return (CCHAR)Position;
    }
#else
    if (BitScanForward(&Position, (ULONG)Value))
    {
        return (CCHAR)Position;
    }
    else if (BitScanForward(&Position, Value >> _BITCOUNT))
    {
        return (CCHAR)(Position + _BITCOUNT);
    }
#endif
    return -1;
}
#endif /* !USE_RTL_BITMAP64 */

VOID
NTAPI
RtlInitializeBitMap(
    _Out_ PRTL_BITMAP BitMapHeader,
    _In_opt_ __drv_aliasesMem PBITMAP_BUFFER BitMapBuffer,
    _In_opt_ ULONG SizeOfBitMap)
{
    /* Setup the bitmap header */
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer = BitMapBuffer;
}

VOID
NTAPI
RtlClearAllBits(
    _In_ PRTL_BITMAP BitMapHeader)
{
    BITMAP_INDEX LengthInUlongs;

    LengthInUlongs = (BitMapHeader->SizeOfBitMap + _BITCOUNT - 1) / _BITCOUNT;
    RtlFillMemoryUlong(BitMapHeader->Buffer, LengthInUlongs * sizeof(BITMAP_INDEX), 0);
}

VOID
NTAPI
RtlSetAllBits(
    _In_ PRTL_BITMAP BitMapHeader)
{
    BITMAP_INDEX LengthInUlongs;

    LengthInUlongs = (BitMapHeader->SizeOfBitMap + _BITCOUNT - 1) / _BITCOUNT;
    RtlFillMemoryUlong(BitMapHeader->Buffer, LengthInUlongs * sizeof(BITMAP_INDEX), ~0);
}

VOID
NTAPI
RtlClearBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX BitNumber)
{
    ASSERT(BitNumber <= BitMapHeader->SizeOfBitMap);
    BitMapHeader->Buffer[BitNumber / _BITCOUNT] &= ~(1 << (BitNumber & (_BITCOUNT - 1)));
}

VOID
NTAPI
RtlSetBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) BITMAP_INDEX BitNumber)
{
    ASSERT(BitNumber <= BitMapHeader->SizeOfBitMap);
    BitMapHeader->Buffer[BitNumber / _BITCOUNT] |= ((BITMAP_INDEX)1 << (BitNumber & (_BITCOUNT - 1)));
}

VOID
NTAPI
RtlClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) BITMAP_INDEX StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) BITMAP_INDEX NumberToClear)
{
    BITMAP_INDEX Bits, Mask;
    PBITMAP_BUFFER Buffer;

    ASSERT(StartingIndex + NumberToClear <= BitMapHeader->SizeOfBitMap);

    /* Calculate buffer start and first bit index */
    Buffer = &BitMapHeader->Buffer[StartingIndex / _BITCOUNT];
    Bits = StartingIndex & (_BITCOUNT - 1);

    /* Are we unaligned? */
    if (Bits)
    {
        /* Create an inverse mask by shifting MAXINDEX */
        Mask = MAXINDEX << Bits;

        /* This is what's left in the first ULONG */
        Bits = _BITCOUNT - Bits;

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
    Buffer += NumberToClear / _BITCOUNT;

    /* Clear what's left */
    NumberToClear &= (_BITCOUNT - 1);
    if (NumberToClear != 0)
    {
        Mask = MAXINDEX << NumberToClear;
        *Buffer &= Mask;
    }
}

VOID
NTAPI
RtlSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) BITMAP_INDEX StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) BITMAP_INDEX NumberToSet)
{
    BITMAP_INDEX Bits, Mask;
    PBITMAP_BUFFER Buffer;

    ASSERT(StartingIndex + NumberToSet <= BitMapHeader->SizeOfBitMap);

    /* Calculate buffer start and first bit index */
    Buffer = &BitMapHeader->Buffer[StartingIndex / _BITCOUNT];
    Bits = StartingIndex & (_BITCOUNT - 1);

    /* Are we unaligned? */
    if (Bits)
    {
        /* Create a mask by shifting MAXINDEX */
        Mask = MAXINDEX << Bits;

        /* This is what's left in the first ULONG */
        Bits = _BITCOUNT - Bits;

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
    RtlFillMemoryUlong(Buffer, NumberToSet >> 3, MAXINDEX);
    Buffer += NumberToSet / _BITCOUNT;

    /* Set what's left */
    NumberToSet &= (_BITCOUNT - 1);
    if (NumberToSet != 0)
    {
        Mask = MAXINDEX << NumberToSet;
        *Buffer |= ~Mask;
    }
}

BOOLEAN
NTAPI
RtlTestBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) BITMAP_INDEX BitNumber)
{
    ASSERT(BitNumber < BitMapHeader->SizeOfBitMap);
    return (BitMapHeader->Buffer[BitNumber / _BITCOUNT] >> (BitNumber & (_BITCOUNT - 1))) & 1;
}

BOOLEAN
NTAPI
RtlAreBitsClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX StartingIndex,
    _In_ BITMAP_INDEX Length)
{
    /* Verify parameters */
    if ((StartingIndex + Length > BitMapHeader->SizeOfBitMap) ||
        (StartingIndex + Length <= StartingIndex))
        return FALSE;

    return RtlpGetLengthOfRunClear(BitMapHeader, StartingIndex, Length) >= Length;
}

BOOLEAN
NTAPI
RtlAreBitsSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX StartingIndex,
    _In_ BITMAP_INDEX Length)
{
    /* Verify parameters */
    if ((StartingIndex + Length > BitMapHeader->SizeOfBitMap) ||
        (StartingIndex + Length <= StartingIndex))
        return FALSE;

    return RtlpGetLengthOfRunSet(BitMapHeader, StartingIndex, Length) >= Length;
}

BITMAP_INDEX
NTAPI
RtlNumberOfSetBits(
    _In_ PRTL_BITMAP BitMapHeader)
{
    PUCHAR Byte, MaxByte;
    BITMAP_INDEX BitCount = 0;
    ULONG Shift;

    Byte = (PUCHAR)BitMapHeader->Buffer;
    MaxByte = Byte + BitMapHeader->SizeOfBitMap / 8;

    while (Byte < MaxByte)
    {
        BitCount += BitCountTable[*Byte++];
    }

    if (BitMapHeader->SizeOfBitMap & 7)
    {
        Shift = 8 - (BitMapHeader->SizeOfBitMap & 7);
        BitCount += BitCountTable[((*Byte) << Shift) & 0xFF];
    }

    return BitCount;
}

BITMAP_INDEX
NTAPI
RtlNumberOfClearBits(
    _In_ PRTL_BITMAP BitMapHeader)
{
    /* Do some math */
    return BitMapHeader->SizeOfBitMap - RtlNumberOfSetBits(BitMapHeader);
}

BITMAP_INDEX
NTAPI
RtlFindClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX NumberToFind,
    _In_ BITMAP_INDEX HintIndex)
{
    BITMAP_INDEX CurrentBit, Margin, CurrentLength;

    /* Check for valid parameters */
    if (!BitMapHeader || NumberToFind > BitMapHeader->SizeOfBitMap)
    {
        return MAXINDEX;
    }

    /* Check if the hint is outside the bitmap */
    if (HintIndex >= BitMapHeader->SizeOfBitMap) HintIndex = 0;

    /* Check for trivial case */
    if (NumberToFind == 0)
    {
        /* Return hint rounded down to byte margin */
        return HintIndex & ~7;
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
                                            MAXINDEX);

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
    return MAXINDEX;
}

BITMAP_INDEX
NTAPI
RtlFindSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX NumberToFind,
    _In_ BITMAP_INDEX HintIndex)
{
    BITMAP_INDEX CurrentBit, Margin, CurrentLength;

    /* Check for valid parameters */
    if (!BitMapHeader || NumberToFind > BitMapHeader->SizeOfBitMap)
    {
        return MAXINDEX;
    }

    /* Check if the hint is outside the bitmap */
    if (HintIndex >= BitMapHeader->SizeOfBitMap) HintIndex = 0;

    /* Check for trivial case */
    if (NumberToFind == 0)
    {
        /* Return hint rounded down to byte margin */
        return HintIndex & ~7;
    }

    /* First margin is end of bitmap */
    Margin = BitMapHeader->SizeOfBitMap;

retry:
    /* Start with hint index, length is 0 */
    CurrentBit = HintIndex;

    /* Loop until something is found or the end is reached */
    while (CurrentBit + NumberToFind <= Margin)
    {
        /* Search for the next set run, by skipping a clear run */
        CurrentBit += RtlpGetLengthOfRunClear(BitMapHeader,
                                              CurrentBit,
                                              MAXINDEX);

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

    /* Did we start at a hint? */
    if (HintIndex)
    {
        /* Retry at the start */
        Margin = min(HintIndex + NumberToFind, BitMapHeader->SizeOfBitMap);
        HintIndex = 0;
        goto retry;
    }

    /* Nothing found */
    return MAXINDEX;
}

BITMAP_INDEX
NTAPI
RtlFindClearBitsAndSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX NumberToFind,
    _In_ BITMAP_INDEX HintIndex)
{
    BITMAP_INDEX Position;

    /* Try to find clear bits */
    Position = RtlFindClearBits(BitMapHeader, NumberToFind, HintIndex);

    /* Did we get something? */
    if (Position != MAXINDEX)
    {
        /* Yes, set the bits */
        RtlSetBits(BitMapHeader, Position, NumberToFind);
    }

    /* Return what we found */
    return Position;
}

BITMAP_INDEX
NTAPI
RtlFindSetBitsAndClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX NumberToFind,
    _In_ BITMAP_INDEX HintIndex)
{
    BITMAP_INDEX Position;

    /* Try to find set bits */
    Position = RtlFindSetBits(BitMapHeader, NumberToFind, HintIndex);

    /* Did we get something? */
    if (Position != MAXINDEX)
    {
        /* Yes, clear the bits */
        RtlClearBits(BitMapHeader, Position, NumberToFind);
    }

    /* Return what we found */
    return Position;
}

BITMAP_INDEX
NTAPI
RtlFindNextForwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX FromIndex,
    _Out_ PBITMAP_INDEX StartingRunIndex)
{
    BITMAP_INDEX Length;

    /* Check for buffer overrun */
    if (FromIndex >= BitMapHeader->SizeOfBitMap)
    {
        *StartingRunIndex = FromIndex;
        return 0;
    }

    /* Assume a set run first, count it's length */
    Length = RtlpGetLengthOfRunSet(BitMapHeader, FromIndex, MAXINDEX);
    *StartingRunIndex = FromIndex + Length;

    /* Now return the length of the run */
    return RtlpGetLengthOfRunClear(BitMapHeader, FromIndex + Length, MAXINDEX);
}

BITMAP_INDEX
NTAPI
RtlFindNextForwardRunSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX FromIndex,
    _Out_ PBITMAP_INDEX StartingRunIndex)
{
    BITMAP_INDEX Length;

    /* Check for buffer overrun */
    if (FromIndex >= BitMapHeader->SizeOfBitMap)
    {
        *StartingRunIndex = FromIndex;
        return 0;
    }

    /* Assume a clear run first, count it's length */
    Length = RtlpGetLengthOfRunClear(BitMapHeader, FromIndex, MAXINDEX);
    *StartingRunIndex = FromIndex + Length;

    /* Now return the length of the run */
    return RtlpGetLengthOfRunSet(BitMapHeader, FromIndex + Length, MAXINDEX);
}

BITMAP_INDEX
NTAPI
RtlFindFirstRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PBITMAP_INDEX StartingIndex)
{
    return RtlFindNextForwardRunClear(BitMapHeader, 0, StartingIndex);
}

BITMAP_INDEX
NTAPI
RtlFindLastBackwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ BITMAP_INDEX FromIndex,
    _Out_ PBITMAP_INDEX StartingRunIndex)
{
    BITMAP_INDEX Value, InvValue, BitPos;
    PBITMAP_BUFFER Buffer;

    /* Make sure we don't go past the end */
    FromIndex = min(FromIndex, BitMapHeader->SizeOfBitMap - 1);

    /* Calculate positions */
    Buffer = BitMapHeader->Buffer + FromIndex / _BITCOUNT;
    BitPos = (_BITCOUNT - 1) - (FromIndex & (_BITCOUNT - 1));

    /* Get the inversed value, clear bits that don't belong to the run */
    InvValue = ~(*Buffer--) << BitPos >> BitPos;

    /* Skip all set ULONGs */
    while (InvValue == 0)
    {
        /* Did we already reach past the first ULONG? */
        if (Buffer < BitMapHeader->Buffer)
        {
            /* Yes, nothing found */
            return 0;
        }

        InvValue = ~(*Buffer--);
    }

    /* We hit a clear bit, check how many set bits are left */
    BitScanReverse(&BitPos, InvValue);

    /* Calculate last bit position */
    FromIndex = (BITMAP_INDEX)((Buffer + 1 - BitMapHeader->Buffer) * _BITCOUNT + BitPos);

    Value = ~InvValue << ((_BITCOUNT - 1) - BitPos) >> ((_BITCOUNT - 1) - BitPos);

    /* Skip all clear ULONGs */
    while (Value == 0 && Buffer >= BitMapHeader->Buffer)
    {
        Value = *Buffer--;
    }

    if (Value != 0)
    {
        /* We hit a set bit, check how many clear bits are left */
        BitScanReverse(&BitPos, Value);

        /* Calculate Starting Index */
        *StartingRunIndex = (BITMAP_INDEX)((Buffer + 1 - BitMapHeader->Buffer) * _BITCOUNT + BitPos + 1);
    }
    else
    {
        /* We reached the start of the bitmap */
        *StartingRunIndex = 0;
    }

    /* Return length of the run */
    return (FromIndex - *StartingRunIndex);
}


ULONG
NTAPI
RtlFindClearRuns(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ PRTL_BITMAP_RUN RunArray,
    _In_ ULONG SizeOfRunArray,
    _In_ BOOLEAN LocateLongestRuns)
{
    BITMAP_INDEX StartingIndex, NumberOfBits, FromIndex = 0, SmallestRun = 0;
    ULONG Run;

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

BITMAP_INDEX
NTAPI
RtlFindLongestRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN PBITMAP_INDEX StartingIndex)
{
    BITMAP_INDEX NumberOfBits, Index, MaxNumberOfBits = 0, FromIndex = 0;

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

BITMAP_INDEX
NTAPI
RtlFindLongestRunSet(
    IN PRTL_BITMAP BitMapHeader,
    IN PBITMAP_INDEX StartingIndex)
{
    BITMAP_INDEX NumberOfBits, Index, MaxNumberOfBits = 0, FromIndex = 0;

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


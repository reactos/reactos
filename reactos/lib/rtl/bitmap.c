/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/bitmap.c
 * PURPOSE:         Bitmap functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* MACROS *******************************************************************/

/* Bits set from LSB to MSB; used as mask for runs < 8 bits */
static const BYTE NTDLL_maskBits[8] = { 0, 1, 3, 7, 15, 31, 63, 127 };

/* Number of set bits for each value of a nibble; used for counting */
static const BYTE NTDLL_nibbleBitCount[16] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/* First set bit in a nibble; used for determining least significant bit */
static const BYTE NTDLL_leastSignificant[16] = {
  0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/* Last set bit in a nibble; used for determining most significant bit */
static const signed char NTDLL_mostSignificant[16] = {
  -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
};

/* PRIVATE FUNCTIONS *********************************************************/

static
int
__cdecl
NTDLL_RunSortFn(const void *lhs,
                const void *rhs)
{
  if (((const RTL_BITMAP_RUN*)lhs)->NumberOfBits > ((const RTL_BITMAP_RUN*)rhs)->NumberOfBits)
    return -1;
  return 1;
}

static
ULONG
WINAPI
NTDLL_FindRuns(PRTL_BITMAP lpBits,
               PRTL_BITMAP_RUN lpSeries,
               ULONG ulCount,
               BOOLEAN bLongest,
               ULONG (*fn)(PRTL_BITMAP,ULONG,PULONG))
{
  BOOL bNeedSort = ulCount > 1 ? TRUE : FALSE;
  ULONG ulPos = 0, ulRuns = 0;

  if (!ulCount)
    return ~0U;

  while (ulPos < lpBits->SizeOfBitMap)
  {
    /* Find next set/clear run */
    ULONG ulSize, ulNextPos = fn(lpBits, ulPos, &ulSize);

    if (ulNextPos == ~0U)
      break;

    if (bLongest && ulRuns == ulCount)
    {
      /* Sort runs with shortest at end, if they are out of order */
      if (bNeedSort)
        qsort(lpSeries, ulRuns, sizeof(RTL_BITMAP_RUN), NTDLL_RunSortFn);

      /* Replace last run if this one is bigger */
      if (ulSize > lpSeries[ulRuns - 1].NumberOfBits)
      {
        lpSeries[ulRuns - 1].StartingIndex = ulNextPos;
        lpSeries[ulRuns - 1].NumberOfBits = ulSize;

        /* We need to re-sort the array, _if_ we didn't leave it sorted */
        if (ulRuns > 1 && ulSize > lpSeries[ulRuns - 2].NumberOfBits)
          bNeedSort = TRUE;
      }
    }
    else
    {
      /* Append to found runs */
      lpSeries[ulRuns].StartingIndex = ulNextPos;
      lpSeries[ulRuns].NumberOfBits = ulSize;
      ulRuns++;

      if (!bLongest && ulRuns == ulCount)
        break;
    }
    ulPos = ulNextPos + ulSize;
  }
  return ulRuns;
}

static
ULONG
NTDLL_FindSetRun(PRTL_BITMAP lpBits,
                 ULONG ulStart,
                 PULONG lpSize)
{
  LPBYTE lpOut;
  ULONG ulFoundAt = 0, ulCount = 0;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)lpBits->Buffer) + (ulStart >> 3u);

  while (1)
  {
    /* Check bits in first byte */
    const BYTE bMask = (0xff << (ulStart & 7)) & 0xff;
    const BYTE bFirst = *lpOut & bMask;

    if (bFirst)
    {
      /* Have a set bit in first byte */
      if (bFirst != bMask)
      {
        /* Not every bit is set */
        ULONG ulOffset;

        if (bFirst & 0x0f)
          ulOffset = NTDLL_leastSignificant[bFirst & 0x0f];
        else
          ulOffset = 4 + NTDLL_leastSignificant[bFirst >> 4];
        ulStart += ulOffset;
        ulFoundAt = ulStart;
        for (;ulOffset < 8; ulOffset++)
        {
          if (!(bFirst & (1 << ulOffset)))
          {
            *lpSize = ulCount;
            return ulFoundAt; /* Set from start, but not until the end */
          }
          ulCount++;
          ulStart++;
        }
        /* Set to the end - go on to count further bits */
        lpOut++;
        break;
      }
      /* every bit from start until the end of the byte is set */
      ulFoundAt = ulStart;
      ulCount = 8 - (ulStart & 7);
      ulStart = (ulStart & ~7u) + 8;
      lpOut++;
      break;
    }
    ulStart = (ulStart & ~7u) + 8;
    lpOut++;
    if (ulStart >= lpBits->SizeOfBitMap)
      return ~0U;
  }

  /* Count blocks of 8 set bits */
  while (*lpOut == 0xff)
  {
    ulCount += 8;
    ulStart += 8;
    if (ulStart >= lpBits->SizeOfBitMap)
    {
      *lpSize = ulCount - (ulStart - lpBits->SizeOfBitMap);
      return ulFoundAt;
    }
    lpOut++;
  }

  /* Count remaining contiguous bits, if any */
  if (*lpOut & 1)
  {
    ULONG ulOffset = 0;

    for (;ulOffset < 7u; ulOffset++)
    {
      if (!(*lpOut & (1 << ulOffset)))
        break;
      ulCount++;
    }
  }
  *lpSize = ulCount;
  return ulFoundAt;
}

static
ULONG
NTDLL_FindClearRun(PRTL_BITMAP lpBits,
                   ULONG ulStart,
                   PULONG lpSize)
{
  LPBYTE lpOut;
  ULONG ulFoundAt = 0, ulCount = 0;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)lpBits->Buffer) + (ulStart >> 3u);

  while (1)
  {
    /* Check bits in first byte */
    const BYTE bMask = (0xff << (ulStart & 7)) & 0xff;
    const BYTE bFirst = (~*lpOut) & bMask;

    if (bFirst)
    {
      /* Have a clear bit in first byte */
      if (bFirst != bMask)
      {
        /* Not every bit is clear */
        ULONG ulOffset;

        if (bFirst & 0x0f)
          ulOffset = NTDLL_leastSignificant[bFirst & 0x0f];
        else
          ulOffset = 4 + NTDLL_leastSignificant[bFirst >> 4];
        ulStart += ulOffset;
        ulFoundAt = ulStart;
        for (;ulOffset < 8; ulOffset++)
        {
          if (!(bFirst & (1 << ulOffset)))
          {
            *lpSize = ulCount;
            return ulFoundAt; /* Clear from start, but not until the end */
          }
          ulCount++;
          ulStart++;
        }
        /* Clear to the end - go on to count further bits */
        lpOut++;
        break;
      }
      /* Every bit from start until the end of the byte is clear */
      ulFoundAt = ulStart;
      ulCount = 8 - (ulStart & 7);
      ulStart = (ulStart & ~7u) + 8;
      lpOut++;
      break;
    }
    ulStart = (ulStart & ~7u) + 8;
    lpOut++;
    if (ulStart >= lpBits->SizeOfBitMap)
      return ~0U;
  }

  /* Count blocks of 8 clear bits */
  while (!*lpOut)
  {
    ulCount += 8;
    ulStart += 8;
    if (ulStart >= lpBits->SizeOfBitMap)
    {
      *lpSize = ulCount - (ulStart - lpBits->SizeOfBitMap);
      return ulFoundAt;
    }
    lpOut++;
  }

  /* Count remaining contiguous bits, if any */
  if (!(*lpOut & 1))
  {
    ULONG ulOffset = 0;

    for (;ulOffset < 7u; ulOffset++)
    {
      if (*lpOut & (1 << ulOffset))
        break;
      ulCount++;
    }
  }
  *lpSize = ulCount;
  return ulFoundAt;
}

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlInitializeBitMap(IN PRTL_BITMAP BitMapHeader,
                    IN PULONG BitMapBuffer,
                    IN ULONG SizeOfBitMap)
{
    /* Setup the bitmap header */
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer = BitMapBuffer;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlAreBitsClear(IN PRTL_BITMAP BitMapHeader,
                IN ULONG StartingIndex,
                IN ULONG Length)
{
  LPBYTE lpOut;
  ULONG ulRemainder;

  if (!BitMapHeader || !Length ||
      StartingIndex >= BitMapHeader->SizeOfBitMap ||
      Length > BitMapHeader->SizeOfBitMap - StartingIndex)
    return FALSE;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)BitMapHeader->Buffer) + (StartingIndex >> 3u);

  /* Check bits in first byte, if StartingIndex isn't a byte boundary */
  if (StartingIndex & 7)
  {
    if (Length > 7)
    {
      /* Check from start bit to the end of the byte */
      if (*lpOut & ((0xff << (StartingIndex & 7)) & 0xff))
        return FALSE;
      lpOut++;
      Length -= (8 - (StartingIndex & 7));
    }
    else
    {
      /* Check from the start bit, possibly into the next byte also */
      USHORT initialWord = NTDLL_maskBits[Length] << (StartingIndex & 7);

      if (*lpOut & (initialWord & 0xff))
        return FALSE;
      if ((initialWord & 0xff00) && (lpOut[1] & (initialWord >> 8)))
        return FALSE;
      return TRUE;
    }
  }

  /* Check bits in blocks of 8 bytes */
  ulRemainder = Length & 7;
  Length >>= 3;
  while (Length--)
  {
    if (*lpOut++)
      return FALSE;
  }

  /* Check remaining bits, if any */
  if (ulRemainder && *lpOut & NTDLL_maskBits[ulRemainder])
    return FALSE;
  return TRUE;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlAreBitsSet(PRTL_BITMAP BitMapHeader,
	      ULONG StartingIndex,
	      ULONG Length)
{
  LPBYTE lpOut;
  ULONG ulRemainder;


  if (!BitMapHeader || !Length ||
      StartingIndex >= BitMapHeader->SizeOfBitMap ||
      Length > BitMapHeader->SizeOfBitMap - StartingIndex)
    return FALSE;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)BitMapHeader->Buffer) + (StartingIndex >> 3u);

  /* Check bits in first byte, if StartingIndex isn't a byte boundary */
  if (StartingIndex & 7)
  {
    if (Length > 7)
    {
      /* Check from start bit to the end of the byte */
      if ((*lpOut &
          ((0xff << (StartingIndex & 7))) & 0xff) != ((0xff << (StartingIndex & 7) & 0xff)))
        return FALSE;
      lpOut++;
      Length -= (8 - (StartingIndex & 7));
    }
    else
    {
      /* Check from the start bit, possibly into the next byte also */
      USHORT initialWord = NTDLL_maskBits[Length] << (StartingIndex & 7);

      if ((*lpOut & (initialWord & 0xff)) != (initialWord & 0xff))
        return FALSE;
      if ((initialWord & 0xff00) &&
          ((lpOut[1] & (initialWord >> 8)) != (initialWord >> 8)))
        return FALSE;
      return TRUE;
    }
  }

  /* Check bits in blocks of 8 bytes */
  ulRemainder = Length & 7;
  Length >>= 3;
  while (Length--)
  {
    if (*lpOut++ != 0xff)
      return FALSE;
  }

  /* Check remaining bits, if any */
  if (ulRemainder &&
      (*lpOut & NTDLL_maskBits[ulRemainder]) != NTDLL_maskBits[ulRemainder])
    return FALSE;
  return TRUE;
}


/*
 * @implemented
 */
VOID NTAPI
RtlClearAllBits(IN OUT PRTL_BITMAP BitMapHeader)
{
    memset(BitMapHeader->Buffer, 0, ((BitMapHeader->SizeOfBitMap + 31) & ~31) >> 3);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlClearBit(PRTL_BITMAP BitMapHeader,
            ULONG BitNumber)
{
    PUCHAR Ptr;

    if (BitNumber >= BitMapHeader->SizeOfBitMap) return;

    Ptr = (PUCHAR)BitMapHeader->Buffer + (BitNumber / 8);
    *Ptr &= ~(1 << (BitNumber % 8));
}

/*
 * @implemented
 */
VOID
NTAPI
RtlClearBits(IN PRTL_BITMAP BitMapHeader,
             IN ULONG StartingIndex,
             IN ULONG NumberToClear)
{
  LPBYTE lpOut;

  if (!BitMapHeader || !NumberToClear ||
      StartingIndex >= BitMapHeader->SizeOfBitMap ||
      NumberToClear > BitMapHeader->SizeOfBitMap - StartingIndex)
    return;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)BitMapHeader->Buffer) + (StartingIndex >> 3u);

  /* Clear bits in first byte, if StartingIndex isn't a byte boundary */
  if (StartingIndex & 7)
  {
    if (NumberToClear > 7)
    {
      /* Clear from start bit to the end of the byte */
      *lpOut++ &= ~(0xff << (StartingIndex & 7));
      NumberToClear -= (8 - (StartingIndex & 7));
    }
    else
    {
      /* Clear from the start bit, possibly into the next byte also */
      USHORT initialWord = ~(NTDLL_maskBits[NumberToClear] << (StartingIndex & 7));

      *lpOut++ &= (initialWord & 0xff);
      *lpOut &= (initialWord >> 8);
      return;
    }
  }

  /* Clear bits (in blocks of 8) on whole byte boundaries */
  if (NumberToClear >> 3)
  {
    memset(lpOut, 0, NumberToClear >> 3);
    lpOut = lpOut + (NumberToClear >> 3);
  }

  /* Clear remaining bits, if any */
  if (NumberToClear & 0x7)
    *lpOut &= ~NTDLL_maskBits[NumberToClear & 0x7];
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindClearBits(PRTL_BITMAP BitMapHeader,
		 ULONG NumberToFind,
		 ULONG HintIndex)
{
  ULONG ulPos, ulEnd;

  if (!BitMapHeader || !NumberToFind || NumberToFind > BitMapHeader->SizeOfBitMap)
    return ~0U;

  ulEnd = BitMapHeader->SizeOfBitMap;

  if (HintIndex + NumberToFind > BitMapHeader->SizeOfBitMap)
    HintIndex = 0;

  ulPos = HintIndex;

  while (ulPos < ulEnd)
  {
    /* FIXME: This could be made a _lot_ more efficient */
    if (RtlAreBitsClear(BitMapHeader, ulPos, NumberToFind))
      return ulPos;

    /* Start from the beginning if we hit the end and started from HintIndex */
    if (ulPos == ulEnd - 1 && HintIndex)
    {
      ulEnd = HintIndex;
      ulPos = HintIndex = 0;
    }
    else
      ulPos++;
  }
  return ~0U;
}

/*
 * @implemented
 */
ULONG NTAPI
RtlFindClearRuns(PRTL_BITMAP BitMapHeader,
		 PRTL_BITMAP_RUN RunArray,
		 ULONG SizeOfRunArray,
		 BOOLEAN LocateLongestRuns)
{
    return NTDLL_FindRuns(BitMapHeader, RunArray, SizeOfRunArray, LocateLongestRuns, NTDLL_FindClearRun);
}

/*
 * @unimplemented
 */
ULONG NTAPI
RtlFindLastBackwardRunClear(IN PRTL_BITMAP BitMapHeader,
			    IN ULONG FromIndex,
			    IN PULONG StartingRunIndex)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @implemented
 */
ULONG NTAPI
RtlFindNextForwardRunClear(IN PRTL_BITMAP BitMapHeader,
			   IN ULONG FromIndex,
			   IN PULONG StartingRunIndex)
{
  ULONG ulSize = 0;

  if (BitMapHeader && FromIndex < BitMapHeader->SizeOfBitMap && StartingRunIndex)
    *StartingRunIndex = NTDLL_FindClearRun(BitMapHeader, FromIndex, &ulSize);

  return ulSize;
}

/*
 * @implemented
 */
ULONG NTAPI
RtlFindFirstRunSet(IN PRTL_BITMAP BitMapHeader,
		   IN PULONG StartingIndex)
{
  ULONG Size;
  ULONG Index;
  ULONG Count;
  PULONG Ptr;
  ULONG  Mask;

  Size = BitMapHeader->SizeOfBitMap;
  if (*StartingIndex > Size)
  {
    *StartingIndex = (ULONG)-1;
    return 0;
  }

  Index = *StartingIndex;
  Ptr = (PULONG)BitMapHeader->Buffer + (Index / 32);
  Mask = 1 << (Index & 0x1F);

  /* Skip clear bits */
  for (; Index < Size && ~*Ptr & Mask; Index++)
  {
    Mask <<= 1;

    if (Mask == 0)
    {
      Mask = 1;
      Ptr++;
    }
  }

  /* Return index of first set bit */
  if (Index >= Size)
  {
    *StartingIndex = (ULONG)-1;
    return 0;
  }
  else
  {
    *StartingIndex = Index;
  }

  /* Count set bits */
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
ULONG NTAPI
RtlFindLongestRunClear(PRTL_BITMAP BitMapHeader,
		       PULONG StartingIndex)
{
  /* GCC complaints that it may be used uninitialized */
  RTL_BITMAP_RUN br = { 0, 0 };

  if (RtlFindClearRuns(BitMapHeader, &br, 1, TRUE) == 1)
  {
    if (StartingIndex)
      *StartingIndex = br.StartingIndex;
    return br.NumberOfBits;
  }
  return 0;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindLongestRunSet(PRTL_BITMAP BitMapHeader,
		     PULONG StartingIndex)
{
  /* GCC complaints that it may be used uninitialized */
  RTL_BITMAP_RUN br = { 0, 0 };

  if (NTDLL_FindRuns(BitMapHeader, &br, 1, TRUE, NTDLL_FindSetRun) == 1)
  {
    if (StartingIndex)
      *StartingIndex = br.StartingIndex;
    return br.NumberOfBits;
  }
  return 0;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindSetBits(PRTL_BITMAP BitMapHeader,
	       ULONG NumberToFind,
	       ULONG HintIndex)
{
  ULONG ulPos, ulEnd;

  if (!BitMapHeader || !NumberToFind || NumberToFind > BitMapHeader->SizeOfBitMap)
    return ~0U;

  ulEnd = BitMapHeader->SizeOfBitMap;

  if (HintIndex + NumberToFind > BitMapHeader->SizeOfBitMap)
    HintIndex = 0;

  ulPos = HintIndex;

  while (ulPos < ulEnd)
  {
    /* FIXME: This could be made a _lot_ more efficient */
    if (RtlAreBitsSet(BitMapHeader, ulPos, NumberToFind))
      return ulPos;

    /* Start from the beginning if we hit the end and had a hint */
    if (ulPos == ulEnd - 1 && HintIndex)
    {
      ulEnd = HintIndex;
      ulPos = HintIndex = 0;
    }
    else
      ulPos++;
  }
  return ~0U;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindSetBitsAndClear(PRTL_BITMAP BitMapHeader,
		       ULONG NumberToFind,
		       ULONG HintIndex)
{
  ULONG ulPos;

  ulPos = RtlFindSetBits(BitMapHeader, NumberToFind, HintIndex);
  if (ulPos != ~0U)
    RtlClearBits(BitMapHeader, ulPos, NumberToFind);
  return ulPos;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlNumberOfSetBits(PRTL_BITMAP BitMapHeader)
{
  ULONG ulSet = 0;

  if (BitMapHeader)
  {
    LPBYTE lpOut = (BYTE *)BitMapHeader->Buffer;
    ULONG Length, ulRemainder;
    BYTE bMasked;

    Length = BitMapHeader->SizeOfBitMap >> 3;
    ulRemainder = BitMapHeader->SizeOfBitMap & 0x7;

    while (Length--)
    {
      ulSet += NTDLL_nibbleBitCount[*lpOut >> 4];
      ulSet += NTDLL_nibbleBitCount[*lpOut & 0xf];
      lpOut++;
    }

    if (ulRemainder)
    {
      bMasked = *lpOut & NTDLL_maskBits[ulRemainder];
      ulSet += NTDLL_nibbleBitCount[bMasked >> 4];
      ulSet += NTDLL_nibbleBitCount[bMasked & 0xf];
    }
  }
  return ulSet;
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetAllBits(IN OUT PRTL_BITMAP BitMapHeader)
{
  memset(BitMapHeader->Buffer, 0xff, ((BitMapHeader->SizeOfBitMap + 31) & ~31) >> 3);
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetBit(PRTL_BITMAP BitMapHeader,
	  ULONG BitNumber)
{
  PUCHAR Ptr;

  if (BitNumber >= BitMapHeader->SizeOfBitMap)
    return;

  Ptr = (PUCHAR)BitMapHeader->Buffer + (BitNumber / 8);

  *Ptr |= (1 << (BitNumber % 8));
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetBits(PRTL_BITMAP BitMapHeader,
	   ULONG StartingIndex,
	   ULONG NumberToSet)
{
  LPBYTE lpOut;

  if (!BitMapHeader || !NumberToSet ||
      StartingIndex >= BitMapHeader->SizeOfBitMap ||
      NumberToSet > BitMapHeader->SizeOfBitMap - StartingIndex)
    return;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)BitMapHeader->Buffer) + (StartingIndex >> 3u);

  /* Set bits in first byte, if StartingIndex isn't a byte boundary */
  if (StartingIndex & 7)
  {
    if (NumberToSet > 7)
    {
      /* Set from start bit to the end of the byte */
      *lpOut++ |= 0xff << (StartingIndex & 7);
      NumberToSet -= (8 - (StartingIndex & 7));
    }
    else
    {
      /* Set from the start bit, possibly into the next byte also */
      USHORT initialWord = NTDLL_maskBits[NumberToSet] << (StartingIndex & 7);

      *lpOut++ |= (initialWord & 0xff);
      *lpOut |= (initialWord >> 8);
      return;
    }
  }

  /* Set bits up to complete byte count */
  if (NumberToSet >> 3)
  {
    memset(lpOut, 0xff, NumberToSet >> 3);
    lpOut = lpOut + (NumberToSet >> 3);
  }

  /* Set remaining bits, if any */
  *lpOut |= NTDLL_maskBits[NumberToSet & 0x7];
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlTestBit(PRTL_BITMAP BitMapHeader,
	   ULONG BitNumber)
{
  PUCHAR Ptr;

  if (BitNumber >= BitMapHeader->SizeOfBitMap)
    return FALSE;

  Ptr = (PUCHAR)BitMapHeader->Buffer + (BitNumber / 8);

  return (BOOLEAN)(*Ptr & (1 << (BitNumber % 8)));
}

/*
 * @implemented
 */
ULONG NTAPI
RtlFindFirstRunClear(PRTL_BITMAP BitMapHeader,
		     PULONG StartingIndex)
{
    return RtlFindNextForwardRunClear(BitMapHeader, 0, StartingIndex);
}

/*
 * @implemented
 */
ULONG NTAPI
RtlNumberOfClearBits(PRTL_BITMAP BitMapHeader)
{

  if (BitMapHeader)
    return BitMapHeader->SizeOfBitMap - RtlNumberOfSetBits(BitMapHeader);
  return 0;
}

/*
 * @implemented
 */
ULONG NTAPI
RtlFindClearBitsAndSet(PRTL_BITMAP BitMapHeader,
		       ULONG NumberToFind,
		       ULONG HintIndex)
{
  ULONG ulPos;

  ulPos = RtlFindClearBits(BitMapHeader, NumberToFind, HintIndex);
  if (ulPos != ~0U)
    RtlSetBits(BitMapHeader, ulPos, NumberToFind);
  return ulPos;
}

/*
 * @implemented
 */
CCHAR WINAPI RtlFindMostSignificantBit(ULONGLONG ulLong)
{
    signed char ret = 32;
    DWORD dw;

    if (!(dw = (DWORD)(ulLong >> 32)))
    {
        ret = 0;
        dw = (DWORD)ulLong;
    }
    if (dw & 0xffff0000)
    {
        dw >>= 16;
        ret += 16;
    }
    if (dw & 0xff00)
    {
        dw >>= 8;
        ret += 8;
    }
    if (dw & 0xf0)
    {
        dw >>= 4;
        ret += 4;
    }
    return ret + NTDLL_mostSignificant[dw];
}

/*
 * @implemented
 */
CCHAR WINAPI RtlFindLeastSignificantBit(ULONGLONG ulLong)
{
    signed char ret = 0;
    DWORD dw;

    if (!(dw = (DWORD)ulLong))
    {
        ret = 32;
        if (!(dw = (DWORD)(ulLong >> 32))) return -1;
    }
    if (!(dw & 0xffff))
    {
        dw >>= 16;
        ret += 16;
    }
    if (!(dw & 0xff))
    {
        dw >>= 8;
        ret += 8;
    }
    if (!(dw & 0x0f))
    {
        dw >>= 4;
        ret += 4;
    }
    return ret + NTDLL_leastSignificant[dw & 0x0f];
}

/* EOF */

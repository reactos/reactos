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

#define MASK(Count, Shift) \
  ((Count) == 32 ? 0xFFFFFFFF : ~(0xFFFFFFFF << (Count)) << (Shift))


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID NTAPI
RtlInitializeBitMap(PRTL_BITMAP BitMapHeader,
		    PULONG BitMapBuffer,
		    ULONG SizeOfBitMap)
{
  BitMapHeader->SizeOfBitMap = SizeOfBitMap;
  BitMapHeader->Buffer = BitMapBuffer;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlAreBitsClear(PRTL_BITMAP BitMapHeader,
		ULONG StartingIndex,
		ULONG Length)
{
  ULONG Size;
  ULONG Shift;
  ULONG Count;
  PULONG Ptr;

  Size = BitMapHeader->SizeOfBitMap;
  if (StartingIndex >= Size ||
      !Length ||
      (StartingIndex + Length > Size))
     return FALSE;

  Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
  while (Length)
  {
    /* Get bit shift in current ulong */
    Shift = StartingIndex & 0x1F;

    /* Get number of bits to check in current ulong */
    Count = (Length > 32 - Shift) ? 32 - Shift : Length;

    /* Check ulong */
    if (*Ptr++ & MASK(Count, Shift))
      return FALSE;

    Length -= Count;
    StartingIndex += Count;
  }

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
  ULONG Size;
  ULONG Shift;
  ULONG Count;
  PULONG Ptr;

  Size = BitMapHeader->SizeOfBitMap;
  if (StartingIndex >= Size ||
      Length == 0 ||
      (StartingIndex + Length > Size))
    return FALSE;

  Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
  while (Length)
  {
    /* Get bit shift in current ulong */
    Shift = StartingIndex & 0x1F;

    /* Get number of bits to check in current ulong */
    Count = (Length > 32 - Shift) ? 32 - Shift : Length;

    /* Check ulong */
    if (~*Ptr++ & MASK(Count, Shift))
      return FALSE;

    Length -= Count;
    StartingIndex += Count;
  }

  return TRUE;
}


/*
 * @implemented
 *
 * Note: According to the documentation, SizeOfBitmap is in bits, so the
 * ROUND_UP(...) must be divided by the number of bits per byte here.
 * This function is exercised by the whole page allocator in npool.c
 * which is how i came across this error.
 */
VOID NTAPI
RtlClearAllBits(IN OUT PRTL_BITMAP BitMapHeader)
{
   if (BitMapHeader != NULL)
   { 
     if (BitMapHeader->Buffer != NULL)
     {
        memset(BitMapHeader->Buffer,
	           0x00,
	           ROUND_UP(BitMapHeader->SizeOfBitMap, 8) / 8);
     }
   }
}


/*
 * @implemented
 */
VOID NTAPI
RtlClearBit(PRTL_BITMAP BitMapHeader,
	    ULONG BitNumber)
{
  PULONG Ptr;

  if (BitNumber >= BitMapHeader->SizeOfBitMap)
    return;

  Ptr = (PULONG)BitMapHeader->Buffer + (BitNumber / 32);

  *Ptr &= ~(1 << (BitNumber % 32));
}


/*
 * @implemented
 */
VOID NTAPI
RtlClearBits(PRTL_BITMAP BitMapHeader,
	     ULONG StartingIndex,
	     ULONG NumberToClear)
{
  ULONG Size;
  ULONG Count;
  ULONG Shift;
  PULONG Ptr;

  Size = BitMapHeader->SizeOfBitMap;
  if (StartingIndex >= Size || NumberToClear == 0)
    return;

  ASSERT(StartingIndex + NumberToClear <= Size);

  Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
  while (NumberToClear)
  {
    /* Bit shift in current ulong */
    Shift = StartingIndex & 0x1F;

    /* Number of bits to change in current ulong */
    Count = (NumberToClear > 32 - Shift ) ? 32 - Shift : NumberToClear;

    /* Adjust ulong */
    *Ptr++ &= ~MASK(Count, Shift);
    NumberToClear -= Count;
    StartingIndex += Count;
  }
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindClearBits(PRTL_BITMAP BitMapHeader,
		 ULONG NumberToFind,
		 ULONG HintIndex)
{
  ULONG Size;
  ULONG Index;
  ULONG Count;
  PULONG Ptr;
  ULONG Mask;
  ULONG Loop;
  ULONG End;
  ULONG OriginalHint = HintIndex;

  Size = BitMapHeader->SizeOfBitMap;
  if (NumberToFind > Size || NumberToFind == 0)
    return -1;

  if (HintIndex >= Size)
    HintIndex = 0;

  /* Initialize the values to the hint location. */
  Index = HintIndex;
  Ptr = BitMapHeader->Buffer + (Index / 32);
  Mask = 1 << (Index & 0x1F);
  End = Size;

  /* The outer loop does the magic of first searching from the
   * hint to the bitmap end and then going again from beginning
   * of the bitmap to the hint location.
   */
  for (Loop = 0; Loop < 2; Loop++)
  {
    while (HintIndex < End)
    {
      /* Count clear bits */
      for (Count = 0; Index < End && ~*Ptr & Mask; Index++)
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

      /* Skip set bits */
      for (; Index < End && *Ptr & Mask; Index++)
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

    /* Optimalization */
    if (OriginalHint == 0)
      break;

    /* Initialize the values for the beginning -> hint loop. */
    HintIndex = Index = 0;
    End = OriginalHint + NumberToFind - 1;
    End = End > Size ? Size : End;
    Ptr = BitMapHeader->Buffer;
    Mask = 1;
  }

  return (ULONG)-1;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindClearBitsAndSet(PRTL_BITMAP BitMapHeader,
		       ULONG NumberToFind,
		       ULONG HintIndex)
{
  ULONG Index;

  Index = RtlFindClearBits(BitMapHeader,
			   NumberToFind,
			   HintIndex);
  if (Index != (ULONG)-1)
  {
    RtlSetBits(BitMapHeader,
	       Index,
	       NumberToFind);
  }

  return Index;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindFirstRunClear(PRTL_BITMAP BitMapHeader,
		     PULONG StartingIndex)
{
  ULONG Size;
  ULONG Index;
  ULONG Count;
  PULONG Ptr;
  ULONG Mask;

  Size = BitMapHeader->SizeOfBitMap;
  if (*StartingIndex > Size)
  {
    *StartingIndex = (ULONG)-1;
    return 0;
  }

  Index = *StartingIndex;
  Ptr = (PULONG)BitMapHeader->Buffer + (Index / 32);
  Mask = 1 << (Index & 0x1F);

  /* Skip set bits */
  for (; Index < Size && *Ptr & Mask; Index++)
  {
    Mask <<= 1;

    if (Mask == 0)
    {
      Mask = 1;
      Ptr++;
    }
  }

  /* Return index of first clear bit */
  if (Index >= Size)
  {
    *StartingIndex = (ULONG)-1;
    return 0;
  }
  else
  {
    *StartingIndex = Index;
  }

  /* Count clear bits */
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


/*
 * @unimplemented
 */
ULONG NTAPI
RtlFindClearRuns(PRTL_BITMAP BitMapHeader,
		 PRTL_BITMAP_RUN RunArray,
		 ULONG SizeOfRunArray,
		 BOOLEAN LocateLongestRuns)
{
  UNIMPLEMENTED;
  return 0;
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
 * @unimplemented
 */
ULONG NTAPI
RtlFindNextForwardRunClear(IN PRTL_BITMAP BitMapHeader,
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
  ULONG Size;
  PULONG Ptr;
  ULONG Index = 0;
  ULONG Count;
  ULONG Max = 0;
  ULONG Start;
  ULONG Maxstart = 0;
  ULONG  Mask = 1;

  Size = BitMapHeader->SizeOfBitMap;
  Ptr = (PULONG)BitMapHeader->Buffer;

  while (Index < Size)
  {
    Start = Index;

    /* Count clear bits */
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

    /* Skip set bits */
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

  if (StartingIndex != NULL)
  {
    *StartingIndex = Maxstart;
  }

  return Max;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindLongestRunSet(PRTL_BITMAP BitMapHeader,
		     PULONG StartingIndex)
{
  ULONG Size;
  PULONG Ptr;
  ULONG Index = 0;
  ULONG Count;
  ULONG Max = 0;
  ULONG Start;
  ULONG Maxstart = 0;
  ULONG Mask = 1;

  Size = BitMapHeader->SizeOfBitMap;
  Ptr = (PULONG)BitMapHeader->Buffer;

  while (Index < Size)
  {
    Start = Index;

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

    if (Count > Max)
    {
      Max = Count;
      Maxstart = Start;
    }
  }

  if (StartingIndex != NULL)
  {
    *StartingIndex = Maxstart;
  }

  return Max;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindSetBits(PRTL_BITMAP BitMapHeader,
	       ULONG NumberToFind,
	       ULONG HintIndex)
{
  ULONG Size;
  ULONG Index;
  ULONG Count;
  PULONG Ptr;
  ULONG Mask;
  ULONG Loop;
  ULONG End;
  ULONG OriginalHint = HintIndex;

  Size = BitMapHeader->SizeOfBitMap;
  if (NumberToFind > Size || NumberToFind == 0)
    return (ULONG)-1;

  if (HintIndex >= Size)
    HintIndex = 0;

  /* Initialize the values to the hint location. */
  Index = HintIndex;
  Ptr = BitMapHeader->Buffer + (Index / 32);
  Mask = 1 << (Index & 0x1F);
  End = Size;

  /* The outer loop does the magic of first searching from the
   * hint to the bitmap end and then going again from beginning
   * of the bitmap to the hint location.
   */
  for (Loop = 0; Loop < 2; Loop++)
  {
    while (HintIndex < End)
    {
      /* Count set bits */
      for (Count = 0; Index < End && *Ptr & Mask; Index++)
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

      /* Skip clear bits */
      for (; Index < End && ~*Ptr & Mask; Index++)
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

    /* Optimalization */
    if (OriginalHint == 0)
      break;

    /* Initialize the values for the beginning -> hint loop. */
    HintIndex = Index = 0;
    End = OriginalHint + NumberToFind - 1;
    End = End > Size ? Size : End;
    Ptr = BitMapHeader->Buffer;
    Mask = 1;
  }

  return (ULONG)-1;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlFindSetBitsAndClear(PRTL_BITMAP BitMapHeader,
		       ULONG NumberToFind,
		       ULONG HintIndex)
{
  ULONG Index;

  Index = RtlFindSetBits(BitMapHeader,
			 NumberToFind,
			 HintIndex);
  if (Index != (ULONG)-1)
  {
    RtlClearBits(BitMapHeader,
		 Index,
		 NumberToFind);
  }

  return Index;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlNumberOfClearBits(PRTL_BITMAP BitMapHeader)
{
  PULONG Ptr;
  ULONG Size;
  ULONG Index;
  ULONG Count;
  ULONG Mask;

  Size = BitMapHeader->SizeOfBitMap;
  Ptr = (PULONG)BitMapHeader->Buffer;

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
ULONG NTAPI
RtlNumberOfSetBits(PRTL_BITMAP BitMapHeader)
{
  PULONG Ptr;
  ULONG Size;
  ULONG Index;
  ULONG Count;
  ULONG Mask;

  Ptr = (PULONG)BitMapHeader->Buffer;
  Size = BitMapHeader->SizeOfBitMap;
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
 *
 * Note: According to the documentation, SizeOfBitmap is in bits, so the
 * ROUND_UP(...) must be divided by the number of bits per byte here.
 * The companion function, RtlClearAllBits, is exercised by the whole page
 * allocator in npool.c which is how i came across this error.
 */
VOID NTAPI
RtlSetAllBits(IN OUT PRTL_BITMAP BitMapHeader)
{
  memset(BitMapHeader->Buffer,
	 0xFF,
	 ROUND_UP(BitMapHeader->SizeOfBitMap, 8) / 8);
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetBit(PRTL_BITMAP BitMapHeader,
	  ULONG BitNumber)
{
  PULONG Ptr;

  if (BitNumber >= BitMapHeader->SizeOfBitMap)
    return;

  Ptr = (PULONG)BitMapHeader->Buffer + (BitNumber / 32);

  *Ptr |= (1 << (BitNumber % 32));
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetBits(PRTL_BITMAP BitMapHeader,
	   ULONG StartingIndex,
	   ULONG NumberToSet)
{
  ULONG Size;
  ULONG Count;
  ULONG Shift;
  PULONG Ptr;

  Size = BitMapHeader->SizeOfBitMap;
  if (StartingIndex >= Size || NumberToSet == 0)
    return;

  ASSERT(StartingIndex + NumberToSet <= Size);

  Ptr = (PULONG)BitMapHeader->Buffer + (StartingIndex / 32);
  while (NumberToSet)
  {
    /* Bit shift in current ulong */
    Shift = StartingIndex & 0x1F;

    /* Number of bits to change in current ulong */
    Count = (NumberToSet > 32 - Shift) ? 32 - Shift : NumberToSet;

    /* Adjust ulong */
    *Ptr++ |= MASK(Count, Shift);
    NumberToSet -= Count;
    StartingIndex += Count;
  }
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlTestBit(PRTL_BITMAP BitMapHeader,
	   ULONG BitNumber)
{
  PULONG Ptr;

  if (BitNumber >= BitMapHeader->SizeOfBitMap)
    return FALSE;

  Ptr = (PULONG)BitMapHeader->Buffer + (BitNumber / 32);

  return (*Ptr & (1 << (BitNumber % 32)));
}

/* EOF */

/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Helper functions to debug RTL_BITMAP
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <ndk/rtlfuncs.h>

typedef struct _RTL_BITMAP_DBG
{
    union
    {
        struct
        {
            ULONG SizeOfBitMap;
            PULONG Buffer;
        };
        RTL_BITMAP BitMap;
    };
    ULONG NumberOfSetBits;
    ULONG BitmapHash;
} RTL_BITMAP_DBG, *PRTL_BITMAP_DBG;

static inline
ULONG fnv1a_hash(ULONG *data, size_t length)
{
    ULONG hash = 2166136261u; // FNV offset basis (a large prime number)
    for (size_t i = 0; i < length; ++i)
    {
        hash ^= data[i];
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

static inline
ULONG
RtlComputeBitmapHashDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader)
{
    ULONG SizeInBytes = ALIGN_UP_BY(BitMapHeader->BitMap.SizeOfBitMap, 32) / 32;
    return fnv1a_hash(BitMapHeader->BitMap.Buffer, SizeInBytes / sizeof(ULONG));
}

static inline
VOID
RtlValidateBitmapDbg(
    _Inout_ PRTL_BITMAP_DBG BitMapHeader)
{
    ULONG NumberOfSetBits = RtlNumberOfSetBits(&BitMapHeader->BitMap);
    ASSERT(BitMapHeader->NumberOfSetBits == NumberOfSetBits);
    ULONG BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
    ASSERT(BitMapHeader->BitmapHash == BitmapHash);
}

_At_(BitMapHeader->SizeOfBitMap, _Post_equal_to_(SizeOfBitMap))
_At_(BitMapHeader->Buffer, _Post_equal_to_(BitMapBuffer))
static inline
VOID
RtlInitializeBitMapDbg(
    _Out_ PRTL_BITMAP_DBG BitMapHeader,
    _In_opt_ __drv_aliasesMem PULONG BitMapBuffer,
    _In_opt_ ULONG SizeOfBitMap)
{
    RtlInitializeBitMap(&BitMapHeader->BitMap, BitMapBuffer, SizeOfBitMap);
    BitMapHeader->NumberOfSetBits = RtlNumberOfSetBits(&BitMapHeader->BitMap);
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

static inline
BOOLEAN
RtlAreBitsClearDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlAreBitsClear(&BitMapHeader->BitMap, StartingIndex, Length);
}

static inline
BOOLEAN
RtlAreBitsSetDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlAreBitsSet(&BitMapHeader->BitMap, StartingIndex, Length);
}

static inline
VOID
RtlClearAllBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader)
{
    RtlValidateBitmapDbg(BitMapHeader);
    RtlClearAllBits(&BitMapHeader->BitMap);
    BitMapHeader->NumberOfSetBits = 0;
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

static inline
VOID
RtlClearBitDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber)
{
    RtlValidateBitmapDbg(BitMapHeader);
    if (RtlCheckBit(&BitMapHeader->BitMap, BitNumber) == FALSE)
    {
        BitMapHeader->NumberOfSetBits--;
    }
    RtlClearBit(&BitMapHeader->BitMap, BitNumber);
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

static inline
VOID
RtlClearBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear)
{
    RtlValidateBitmapDbg(BitMapHeader);
    RtlClearBits(&BitMapHeader->BitMap, StartingIndex, NumberToClear);
    BitMapHeader->NumberOfSetBits = RtlNumberOfSetBits(&BitMapHeader->BitMap);
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

static inline
ULONG
RtlFindClearBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindClearBits(&BitMapHeader->BitMap, NumberToFind, HintIndex);
}

static inline
ULONG
RtlFindClearBitsAndSetDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindClearBitsAndSet(&BitMapHeader->BitMap, NumberToFind, HintIndex);
}

static inline
ULONG
RtlFindFirstRunClearDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _Out_ PULONG StartingIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindFirstRunClear(&BitMapHeader->BitMap, StartingIndex);
}

static inline
ULONG
RtlFindClearRunsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _Out_writes_to_(SizeOfRunArray, return) PRTL_BITMAP_RUN RunArray,
    _In_range_(>, 0) ULONG SizeOfRunArray,
    _In_ BOOLEAN LocateLongestRuns)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindClearRuns(&BitMapHeader->BitMap, RunArray, SizeOfRunArray, LocateLongestRuns);
}

static inline
ULONG
RtlFindLastBackwardRunClearDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindLastBackwardRunClear(&BitMapHeader->BitMap, FromIndex, StartingRunIndex);
}

static inline
ULONG
RtlFindLongestRunClearDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _Out_ PULONG StartingIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindLongestRunClear(&BitMapHeader->BitMap, StartingIndex);
}

static inline
ULONG
RtlFindNextForwardRunClearDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindNextForwardRunClear(&BitMapHeader->BitMap, FromIndex, StartingRunIndex);
}

static inline
ULONG
RtlFindNextForwardRunSetDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindNextForwardRunSet(&BitMapHeader->BitMap, FromIndex, StartingRunIndex);
}

static inline
ULONG
RtlFindSetBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindSetBits(&BitMapHeader->BitMap, NumberToFind, HintIndex);
}

static inline
ULONG
RtlFindSetBitsAndClearDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlFindSetBitsAndClear(&BitMapHeader->BitMap, NumberToFind, HintIndex);
}

static inline
ULONG
RtlNumberOfClearBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlNumberOfClearBits(&BitMapHeader->BitMap);
}

static inline
ULONG
RtlNumberOfSetBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlNumberOfSetBits(&BitMapHeader->BitMap);
}

static inline
VOID
RtlSetBitDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber)
{
    RtlValidateBitmapDbg(BitMapHeader);
    if (RtlCheckBit(&BitMapHeader->BitMap, BitNumber) == FALSE)
    {
        BitMapHeader->NumberOfSetBits++;
    }
    RtlSetBit(&BitMapHeader->BitMap, BitNumber);
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

static inline
VOID
RtlSetBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet)
{
    RtlValidateBitmapDbg(BitMapHeader);
    RtlSetBits(&BitMapHeader->BitMap, StartingIndex, NumberToSet);
    BitMapHeader->NumberOfSetBits = RtlNumberOfSetBits(&BitMapHeader->BitMap);
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

static inline
VOID
RtlSetAllBitsDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader)
{
    RtlValidateBitmapDbg(BitMapHeader);
    RtlSetAllBits(&BitMapHeader->BitMap);
    BitMapHeader->NumberOfSetBits = BitMapHeader->BitMap.SizeOfBitMap;
    BitMapHeader->BitmapHash = RtlComputeBitmapHashDbg(BitMapHeader);
}

_Must_inspect_result_
static inline
BOOLEAN
RtlTestBitDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlTestBit(&BitMapHeader->BitMap, BitNumber);
}

_Must_inspect_result_
static inline
BOOLEAN
RtlCheckBitDbg(
    _In_ PRTL_BITMAP_DBG BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitPosition)
{
    RtlValidateBitmapDbg(BitMapHeader);
    return RtlCheckBit(&BitMapHeader->BitMap, BitPosition);
}

#define _RTL_BITMAP _RTL_BITMAP_DBG
#define RTL_BITMAP RTL_BITMAP_DBG
#define PRTL_BITMAP PRTL_BITMAP_DBG

#define RtlInitializeBitMap RtlInitializeBitMapDbg
#define RtlAreBitsClear RtlAreBitsClearDbg
#define RtlAreBitsSet RtlAreBitsSetDbg
#define RtlClearAllBits RtlClearAllBitsDbg
#define RtlClearBit RtlClearBitDbg
#define RtlClearBits RtlClearBitsDbg
#define RtlFindClearBits RtlFindClearBitsDbg
#define RtlFindClearBitsAndSet RtlFindClearBitsAndSetDbg
#define RtlFindFirstRunClear RtlFindFirstRunClearDbg
#define RtlFindClearRuns RtlFindClearRunsDbg
#define RtlFindLastBackwardRunClear RtlFindLastBackwardRunClearDbg
#define RtlFindLongestRunClear RtlFindLongestRunClearDbg
#define RtlFindNextForwardRunClear RtlFindNextForwardRunClearDbg
#define RtlFindNextForwardRunSet RtlFindNextForwardRunSetDbg
#define RtlFindSetBits RtlFindSetBitsDbg
#define RtlFindSetBitsAndClear RtlFindSetBitsAndClearDbg
#define RtlNumberOfClearBits RtlNumberOfClearBitsDbg
#define RtlNumberOfSetBits RtlNumberOfSetBitsDbg
#define RtlSetBit RtlSetBitDbg
#define RtlSetBits RtlSetBitsDbg
#define RtlSetAllBits RtlSetAllBitsDbg
#define RtlTestBit RtlTestBitDbg
#undef RtlCheckBit
#define RtlCheckBit RtlCheckBitDbg

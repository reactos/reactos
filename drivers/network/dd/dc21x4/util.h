/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Utility header file
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <pshpack1.h>
typedef struct _ETH_HEADER
{
    UCHAR Destination[ETH_LENGTH_OF_ADDRESS];
    UCHAR Source[ETH_LENGTH_OF_ADDRESS];
    USHORT PayloadType;
} ETH_HEADER, *PETH_HEADER;
#include <poppack.h>

#define ETH_IS_LOCALLY_ADMINISTERED(Address) \
    ((BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x02)))

#define ETH_IS_EMPTY(Address) \
    ((BOOLEAN)((((PUCHAR)(Address))[0] | ((PUCHAR)(Address))[1] | ((PUCHAR)(Address))[2] | \
                ((PUCHAR)(Address))[3] | ((PUCHAR)(Address))[4] | ((PUCHAR)(Address))[5]) == 0))

#if defined(_M_IX86) || defined(_M_AMD64)
/* Strict memory model, does not reorder Write-Write operations */
#define DC_WRITE_BARRIER()    KeMemoryBarrierWithoutFence()
#else
#define DC_WRITE_BARRIER()    KeMemoryBarrier()
#endif

#if defined(_MSC_VER)
/*
 * Merge with PAGE, we don't need a new pageable section. For a small amount of data,
 * there is additional size overhead if the actual data size is smaller than section alignment.
 * GCC doesn't seem to appreciate this idea.
 */
#define DC_PG_DATA  DATA_SEG("PAGE")
#else
#define DC_PG_DATA
#endif

/* Access to unaligned memory */
FORCEINLINE
USHORT
DcRetrieveWord(
    _In_ const VOID* Data)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    /* Supported by ISA */
    return *(const UNALIGNED USHORT*)Data;
#else
    USHORT Result;

    NdisMoveMemory(&Result, Data, sizeof(Result));
    return Result;
#endif
}

#if DBG
#define DcPopEntryList PopEntryList
#else
/*
 * This is an optimized version of the PopEntryList() function.
 * We assume that the next entry has already been checked for nullability
 * so we don't need to.
 */
FORCEINLINE
PSINGLE_LIST_ENTRY
DcPopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead)
{
    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;

    ASSERT(FirstEntry);

    ListHead->Next = FirstEntry->Next;

    return FirstEntry;
}
#endif

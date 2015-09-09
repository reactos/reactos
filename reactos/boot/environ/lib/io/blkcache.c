/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/blkcache.c
 * PURPOSE:         Boot Library Block Cache Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

ULONG BcpBlockAllocatorHandle;
ULONG BcpHashTableId;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BcpDestroy (
    VOID
    )
{
    //BcpPurgeCacheEntries();
    //return BlpMmDeleteBlockAllocator(BcpBlockAllocatorHandle);
    EfiPrintf(L"Destructor for block cache not yet implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
BcpCompareKey (
    _In_ PBL_HASH_ENTRY Entry1,
    _In_ PBL_HASH_ENTRY Entry2
    )
{
    PULONG Value1, Value2;

    Value1 = Entry1->Value;
    Value2 = Entry2->Value;
    return Entry1->Size == Entry2->Size && Entry1->Flags == Entry2->Flags && *Value1 == *Value2 && Value1[1] == Value2[1] && Value1[2] == Value2[2];
}

ULONG
BcpHashFunction (
    _In_ PBL_HASH_ENTRY Entry,
    _In_ ULONG TableSize
    )
{
    ULONG i, j, ValueHash;
    PUCHAR ValueBuffer;

    j = 0;
    ValueHash = 0;
    i = 0;

    ValueBuffer = Entry->Value;

    do
    {
        ValueHash += ValueBuffer[i++];
    } while (i < 8);

    do
    {
        ValueHash += ValueBuffer[j++ + 8];
    } while (j < 4);

    return ValueHash % TableSize;
}

NTSTATUS
BcInitialize (
    VOID
    )
{
    NTSTATUS Status;

    Status = BlHtCreate(50, BcpHashFunction, BcpCompareKey, &BcpHashTableId);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    BcpBlockAllocatorHandle = BlpMmCreateBlockAllocator();
    if (BcpBlockAllocatorHandle == -1)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

    Status = BlpIoRegisterDestroyRoutine(BcpDestroy);
    if (Status >= 0)
    {
        return Status;
    }

Quickie:
    EfiPrintf(L"Failure path not yet implemented\n");
#if 0
    if (BcpHashTableId != -1)
    {
        BlHtDestroy(BcpHashTableId);
    }
    if (BcpBlockAllocatorHandle != -1)
    {
        BlpMmDeleteBlockAllocator(BcpBlockAllocatorHandle);
    }
#endif
    return Status;
}

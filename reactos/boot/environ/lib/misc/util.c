/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/util.c
 * PURPOSE:         Boot Library Utility Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID UtlRsdt;
PVOID UtlXsdt;

PVOID UtlMcContext;
PVOID UtlMcDisplayMessageRoutine;
PVOID UtlMcUpdateMessageRoutine;

PVOID UtlProgressRoutine;
PVOID UtlProgressContext;
PVOID UtlProgressInfoRoutine;
ULONG UtlProgressGranularity;
ULONG UtlCurrentPercentComplete;
ULONG UtlNextUpdatePercentage;
BOOLEAN UtlProgressNeedsInfoUpdate;
PVOID UtlProgressInfo;

/* FUNCTIONS *****************************************************************/

/*++
 * @name EfiGetEfiStatusCode
 *
 *     The EfiGetEfiStatusCode routine converts an NT Status to an EFI status.
 *
 * @param  Status
 *         NT Status code to be converted.
 *
 * @remark Only certain, specific NT status codes are converted to EFI codes.
 *
 * @return The corresponding EFI Status code, EFI_NO_MAPPING otherwise.
 *
 *--*/
EFI_STATUS
EfiGetEfiStatusCode(
    _In_ NTSTATUS Status
    )
{
    switch (Status)
    {
        case STATUS_NOT_SUPPORTED:
            return EFI_UNSUPPORTED;
        case STATUS_DISK_FULL:
            return EFI_VOLUME_FULL;
        case STATUS_INSUFFICIENT_RESOURCES:
            return EFI_OUT_OF_RESOURCES;
        case STATUS_MEDIA_WRITE_PROTECTED:
            return EFI_WRITE_PROTECTED;
        case STATUS_DEVICE_NOT_READY:
            return EFI_NOT_STARTED;
        case STATUS_DEVICE_ALREADY_ATTACHED:
            return EFI_ALREADY_STARTED;
        case STATUS_MEDIA_CHANGED:
            return EFI_MEDIA_CHANGED;
        case STATUS_INVALID_PARAMETER:
            return EFI_INVALID_PARAMETER;
        case STATUS_ACCESS_DENIED:
            return EFI_ACCESS_DENIED;
        case STATUS_BUFFER_TOO_SMALL:
            return EFI_BUFFER_TOO_SMALL;
        case STATUS_DISK_CORRUPT_ERROR:
            return EFI_VOLUME_CORRUPTED;
        case STATUS_REQUEST_ABORTED:
            return EFI_ABORTED;
        case STATUS_NO_MEDIA:
            return EFI_NO_MEDIA;
        case STATUS_IO_DEVICE_ERROR:
            return EFI_DEVICE_ERROR;
        case STATUS_INVALID_BUFFER_SIZE:
            return EFI_BAD_BUFFER_SIZE;
        case STATUS_NOT_FOUND:
            return EFI_NOT_FOUND;
        case STATUS_DRIVER_UNABLE_TO_LOAD:
            return EFI_LOAD_ERROR;
        case STATUS_NO_MATCH:
            return EFI_NO_MAPPING;
        case STATUS_SUCCESS:
            return EFI_SUCCESS;
        case STATUS_TIMEOUT:
            return EFI_TIMEOUT;
        default:
            return EFI_NO_MAPPING;
    }
}

/*++
 * @name EfiGetNtStatusCode
 *
 *     The EfiGetNtStatusCode routine converts an EFI Status to an NT status.
 *
 * @param  EfiStatus
 *         EFI Status code to be converted.
 *
 * @remark Only certain, specific EFI status codes are converted to NT codes.
 *
 * @return The corresponding NT Status code, STATUS_UNSUCCESSFUL otherwise.
 *
 *--*/
NTSTATUS
EfiGetNtStatusCode (
    _In_ EFI_STATUS EfiStatus
    )
{
    switch (EfiStatus)
    {
        case EFI_NOT_READY:
        case EFI_NOT_FOUND:
            return STATUS_NOT_FOUND;
        case EFI_NO_MEDIA:
            return STATUS_NO_MEDIA;
        case EFI_MEDIA_CHANGED:
            return STATUS_MEDIA_CHANGED;
        case EFI_ACCESS_DENIED:
        case EFI_SECURITY_VIOLATION:
            return STATUS_ACCESS_DENIED;
        case EFI_TIMEOUT:
        case EFI_NO_RESPONSE:
            return STATUS_TIMEOUT;
        case EFI_NO_MAPPING:
            return STATUS_NO_MATCH;
        case EFI_NOT_STARTED:
            return STATUS_DEVICE_NOT_READY;
        case EFI_ALREADY_STARTED:
            return STATUS_DEVICE_ALREADY_ATTACHED;
        case EFI_ABORTED:
            return STATUS_REQUEST_ABORTED;
        case EFI_VOLUME_FULL:
            return STATUS_DISK_FULL;
        case EFI_DEVICE_ERROR:
            return STATUS_IO_DEVICE_ERROR;
        case EFI_WRITE_PROTECTED:
            return STATUS_MEDIA_WRITE_PROTECTED;
        /* @FIXME: ReactOS Headers don't yet have this */
        //case EFI_OUT_OF_RESOURCES:
            //return STATUS_INSUFFICIENT_NVRAM_RESOURCES;
        case EFI_VOLUME_CORRUPTED:
            return STATUS_DISK_CORRUPT_ERROR;
        case EFI_BUFFER_TOO_SMALL:
            return STATUS_BUFFER_TOO_SMALL;
        case EFI_SUCCESS:
            return STATUS_SUCCESS;
        case  EFI_LOAD_ERROR:
            return STATUS_DRIVER_UNABLE_TO_LOAD;
        case EFI_INVALID_PARAMETER:
            return STATUS_INVALID_PARAMETER;
        case EFI_UNSUPPORTED:
            return STATUS_NOT_SUPPORTED;
        case EFI_BAD_BUFFER_SIZE:
            return STATUS_INVALID_BUFFER_SIZE;
        default:
            return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
BlUtlInitialize (
    VOID
    )
{
    UtlRsdt = 0;
    UtlXsdt = 0;

    UtlMcContext = 0;
    UtlMcDisplayMessageRoutine = 0;
    UtlMcUpdateMessageRoutine = 0;

    UtlProgressRoutine = 0;
    UtlProgressContext = 0;
    UtlProgressInfoRoutine = 0;
    UtlProgressGranularity = 0;
    UtlCurrentPercentComplete = 0;
    UtlNextUpdatePercentage = 0;
    UtlProgressNeedsInfoUpdate = 0;
    UtlProgressInfo = 0;

    return STATUS_SUCCESS;
}

PVOID
BlTblFindEntry (
    _In_ PVOID *Table,
    _In_ ULONG Count,
    _Out_ PULONG EntryIndex,
    _In_ PBL_TBL_LOOKUP_ROUTINE Callback,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ PVOID Argument3,
    _In_ PVOID Argument4
    )
{
    PVOID Entry = NULL;
    ULONG Index = 0;
    BOOLEAN Result;

    /* Check for invalid parameters */
    if (!(Table) || !(EntryIndex) || !(Count))
    {
        return Entry;
    }

    /* Loop each entry in the table */
    while (Index < Count)
    {
        /* Check if this entry is filled out */
        if (Table[Index])
        {
            /* Call the comparison function */
            Result = Callback(Table[Index],
                              Argument1,
                              Argument2,
                              Argument3,
                              Argument4);
            if (Result)
            {
                /* Entry fouund return it */
                *EntryIndex = Index;
                Entry = Table[Index];
                break;
            }
        }
    }

    /* Return the entry that was (or wasn't) found */
    return Entry;
}

NTSTATUS
BlTblSetEntry (
    _Inout_ PVOID** Table,
    _Inout_ PULONG Count,
    _In_ PVOID Entry,
    _Out_ PULONG EntryIndex,
    _In_ PBL_TBL_SET_ROUTINE Callback
    )
{
    ULONG NewCount;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    PVOID* NewTable;

    /* Make sure all the parameters were specified */
    if (!(Table) || !(*Table) || !(Count) || !(Callback))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Read the current table */
    NewTable = *Table;
    NewCount = *Count;

    /* Iterate over it */
    while (Index < NewCount)
    {
        /* Look for a free index */
        if (!NewTable[Index])
        {
            goto SetIndex;
        }

        /* No free index yet, keep going */
        ++Index;
    }

    /* No free index was found, try to purge some entries */
    Index = 0;
    while (Index < NewCount)
    {
        /* Call each purge callback, trying to make space */
        Status = Callback(NewTable[Index]);
        if (NT_SUCCESS(Status))
        {
            /* We should have this slot available now */
            goto SetIndex;
        }

        /* Keep trying to purge more */
        ++Index;
    }

    /* Double the table */
    NewTable = BlMmAllocateHeap(2 * sizeof(PVOID) * NewCount);
    if (!NewTable)
    {
        return STATUS_NO_MEMORY;
    }

    /* Clear the new table, and copy the old entries */
    RtlZeroMemory(&NewTable[NewCount], sizeof(PVOID) * NewCount);
    RtlCopyMemory(NewTable, *Table, sizeof(PVOID) * NewCount);

    /* Free the old table */
    BlMmFreeHeap(*Table);

    /* Return the new table and count */
    *Count = 2 * NewCount;
    *Table = NewTable;

SetIndex:
    /* Set the index and return */
    NewTable[Index] = Entry;
    *EntryIndex = Index;
    return Status;
}

NTSTATUS
BlTblMap (
    _In_ PVOID *Table,
    _In_ ULONG Count,
    _In_ PBL_TBL_MAP_ROUTINE MapCallback
    )
{
    NTSTATUS Status, LocalStatus;
    PVOID Entry;
    ULONG Index;

    /* Bail out if there's no table */
    if (!Table)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Assume success and loop each index */
    Status = STATUS_SUCCESS;
    for (Index = 0; Index < Count; Index++)
    {
        /* See if an entry exists at this index */
        Entry = Table[Index];
        if (Entry)
        {
            /* Call the map routine for this entry */
            LocalStatus = MapCallback(Entry, Index);
            if (!NT_SUCCESS(LocalStatus))
            {
                /* Propagate failure only */
                Status = LocalStatus;
            }
        }
    }

    /* Return status to caller */
    return Status;
}

ULONG HtTableSize;
PBL_HASH_TABLE* HtTableArray;
ULONG HtTableEntries;

ULONG
DefaultHashFunction (
    _In_ PBL_HASH_ENTRY Entry,
    _In_ ULONG TableSize
    )
{
    PUCHAR Value;
    ULONG KeyHash, i;

    /* Check if the value is a pointer, or embedded inline */
    Value = (Entry->Flags & BL_HT_VALUE_IS_INLINE) ? Entry->Value : (PUCHAR)&Entry->Value;

    /* Iterate over each byte, and sum it */
    for (i = 0, KeyHash = 0; i < Entry->Size; i++)
    {
        KeyHash += Value[i++];
    }

    /* Modulo the number of buckets */
    return KeyHash % TableSize;
}

BOOLEAN
HtpCompareKeys (
    _In_ PBL_HASH_ENTRY Entry1,
    _In_ PBL_HASH_ENTRY Entry2
    )
{
    ULONG Flags;
    BOOLEAN ValueMatch;

    /* Check if the flags or sizes are not matching */
    Flags = Entry1->Flags;
    if ((Entry1->Size != Entry2->Size) || (Flags != Entry2->Flags))
    {
        ValueMatch = FALSE;
    }
    else if (Flags & BL_HT_VALUE_IS_INLINE)
    {
        /* Check if this is an in-line value, compare it */
        ValueMatch = Entry1->Value == Entry2->Value;
    }
    else
    {
        /* This is a pointer value, compare it */
        ValueMatch = (RtlCompareMemory(Entry1->Value, Entry2->Value, Entry1->Size) ==
                      Entry1->Size);
    }

    /* Return if it matched */
    return ValueMatch;
}

NTSTATUS
TblDoNotPurgeEntry (
    _In_ PVOID Entry
    )
{
    /* Never purge this entry */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
BlHtCreate (
    _In_ ULONG Size,
    _In_ PBL_HASH_TABLE_HASH_FUNCTION HashFunction,
    _In_ PBL_HASH_TABLE_COMPARE_FUNCTION CompareFunction,
    _Out_ PULONG Id
    )
{
    NTSTATUS Status;
    PBL_HASH_TABLE HashTable;
    ULONG i;

    /* Assume failure */
    HashTable = NULL;

    /* Can't create a table with no ID */
    if (!Id)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we don't already have a hash table table */
    if (!HtTableSize)
    {
        /* Allocate it and zero it out */
        HtTableSize = 4;
        HtTableArray = BlMmAllocateHeap(HtTableSize * sizeof(PVOID));
        if (!HtTableArray)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }
        RtlZeroMemory(HtTableArray, HtTableSize * sizeof(PVOID));
        HtTableEntries = 0;
    }

    /* Allocate the hash table */
    HashTable = BlMmAllocateHeap(sizeof(*HashTable));
    if (!HashTable)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Fill it out */
    HashTable->HashFunction = HashFunction ? HashFunction : DefaultHashFunction;
    HashTable->CompareFunction = CompareFunction ? CompareFunction : HtpCompareKeys;
    HashTable->Size = Size ? Size : 13;

    /* Allocate the hash links, one for each bucket */
    HashTable->HashLinks = BlMmAllocateHeap(sizeof(LIST_ENTRY) * HashTable->Size);
    if (!HashTable->HashLinks)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Initialize the hash links */
    for (i = 0; i < HashTable->Size; i++)
    {
        InitializeListHead(&HashTable->HashLinks[i]);
    }

    /* Save us in the table of hash tables */
    Status = BlTblSetEntry((PVOID**)&HtTableArray,
                           &Size,
                           HashTable,
                           Id,
                           TblDoNotPurgeEntry);
    if (NT_SUCCESS(Status))
    {
        /* One more -- we're done */
        ++HtTableEntries;
        return Status;
    }

Quickie:
    /* Check if we just allocated the table array now */
    if (!(HtTableEntries) && (HtTableArray))
    {
        /* Free it */
        BlMmFreeHeap(HtTableArray);
        HtTableArray = NULL;
        HtTableSize = 0;
    }

    /* Check if we allocated a hash table*/
    if (HashTable)
    {
        /* With links? */
        if (HashTable->HashLinks)
        {
            /* Free them */
            BlMmFreeHeap(HashTable->HashLinks);
        }

        /* Free the table*/
        BlMmFreeHeap(HashTable);
    }

    /* We're done */
    return Status;
}

NTSTATUS
BlHtLookup (
    _In_ ULONG TableId,
    _In_ PBL_HASH_ENTRY Entry,
    _Out_opt_ PBL_HASH_VALUE *Value
    )
{
    PBL_HASH_TABLE HashTable;
    ULONG HashValue;
    NTSTATUS Status;
    PLIST_ENTRY HashLinkHead, HashLink;
    PBL_HASH_NODE HashNode;

    /* Check if the table ID is invalid, or we have no entry, or it's malformed */
    if ((HtTableSize <= TableId) ||
        !(Entry) ||
        ((Entry->Flags & BL_HT_VALUE_IS_INLINE) && (Entry->Size != sizeof(ULONG))))
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Otherwise, get the hash table for this index */
        HashTable = HtTableArray[TableId];

        /* Get the hash bucket */
        HashValue = HashTable->HashFunction(Entry, HashTable->Size);

        /* Start iterating each entry in the bucket, assuming failure */
        Status = STATUS_NOT_FOUND;
        HashLinkHead = &HashTable->HashLinks[HashValue];
        HashLink = HashLinkHead->Flink;
        while (HashLink != HashLinkHead)
        {
            /* Get a node in this bucket, and compare the value */
            HashNode = CONTAINING_RECORD(HashLink, BL_HASH_NODE, ListEntry);
            if (HashTable->CompareFunction(&HashNode->Entry, Entry))
            {
                /* Does the caller want the value? */
                if (Value)
                {
                    /* Return it */
                    *Value = &HashNode->Value;
                }

                /* Return success and stop scanning */
                Status = STATUS_SUCCESS;
                break;
            }

            /* Try the next node */
            HashLink = HashLink->Flink;
        }
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
BlHtStore (
    _In_ ULONG TableId,
    _In_ PBL_HASH_ENTRY Entry,
    _In_ PVOID Data,
    _In_ ULONG DataSize
    )
{
    PBL_HASH_NODE HashNode;
    NTSTATUS Status;
    PLIST_ENTRY HashLinkHead;
    PBL_HASH_TABLE HashTable;

    /* Check for invalid tablle ID, missing arguments, or malformed entry */
    if ((HtTableSize <= TableId) ||
        !(Entry) ||
        !(Data) ||
        !(Entry->Size) ||
        !(Entry->Value) ||
        !(DataSize) ||
        ((Entry->Flags & BL_HT_VALUE_IS_INLINE) && (Entry->Size != sizeof(ULONG))))
    {
        /* Fail the call */
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Get the hash table for this ID */
    HashTable = HtTableArray[TableId];

    /* Allocate a hash node */
    HashNode = BlMmAllocateHeap(sizeof(*HashNode));
    if (!HashNode)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Capture all the data*/
    HashNode->Entry.Size = Entry->Size;
    HashNode->Entry.Flags = Entry->Flags;
    HashNode->Entry.Value = Entry->Value;
    HashNode->Value.DataSize = DataSize;
    HashNode->Value.Data = Data;

    /* Insert it into the bucket list and return success */
    HashLinkHead = &HashTable->HashLinks[HashTable->HashFunction(Entry, HashTable->Size)];
    InsertTailList(HashLinkHead, &HashNode->ListEntry);
    Status = STATUS_SUCCESS;

Quickie:
    return Status;
}

VOID
BlFwReboot (
    VOID
    )
{
#ifdef BL_KD_SUPPORTED
    /* Stop the boot debugger*/
    BlBdStop();
#endif

    /* Reset the machine */
    EfiResetSystem(EfiResetCold);
}

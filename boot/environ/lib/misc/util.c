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

PRSDT UtlRsdt;
PXSDT UtlXsdt;

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

NTSTATUS
BlUtlGetAcpiTable (
    _Out_ PVOID* TableAddress,
    _In_ ULONG Signature
    )
{
    ULONG i, TableCount, HeaderLength;
    NTSTATUS Status;
    PRSDT Rsdt;
    PXSDT Xsdt;
    PHYSICAL_ADDRESS PhysicalAddress;
    PDESCRIPTION_HEADER Header;

    Header = 0;

    /* Make sure there's an output parameter */
    if (!TableAddress)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the currently known RSDT and XSDT */
    Rsdt = (PRSDT)UtlRsdt;
    Xsdt = (PXSDT)UtlXsdt;

    /* Is there an RSDT? */
    if (!Rsdt)
    {
        /* No -- is there an XSDT? */
        if (!Xsdt)
        {
            /* No. Look up the RSDT */
            Status = EfipGetRsdt(&PhysicalAddress);
            if (!NT_SUCCESS(Status))
            {
                EfiPrintf(L"no rsdp found\r\n");
                return Status;
            }

            /* Map the header */
            Status = BlMmMapPhysicalAddressEx((PVOID)&Header,
                                              0,
                                              sizeof(*Header),
                                              PhysicalAddress);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Unmap the header */
            BlMmUnmapVirtualAddressEx(Header, sizeof(*Header));

            /* Map the whole table */
            Status = BlMmMapPhysicalAddressEx((PVOID)&Header,
                                              0,
                                              Header->Length,
                                              PhysicalAddress);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Check if its an XSDT or an RSDT */
            if (Header->Signature == XSDT_SIGNATURE)
            {
                /* It's an XSDT */
                Xsdt = (PXSDT)Header;
                UtlXsdt = Xsdt;
            }
            else
            {
                /* It's an RSDT */
                Rsdt = (PRSDT)Header;
                UtlRsdt = Rsdt;
            }
        }
    }

    /* OK, so do we have an XSDT after all? */
    if (Xsdt)
    {
        /* Yes... how big is it? */
        HeaderLength = Xsdt->Header.Length;
        if (HeaderLength >= sizeof(*Header))
        {
            HeaderLength = sizeof(*Header);
        }

        /* Based on that, how many tables are there? */
        TableCount = (Xsdt->Header.Length - HeaderLength) / sizeof(PHYSICAL_ADDRESS);
    }
    else
    {
        /* Nope, we have an RSDT. How big is it? */
        HeaderLength = Rsdt->Header.Length;
        if (HeaderLength >= sizeof(*Header))
        {
            HeaderLength = sizeof(*Header);
        }

        /* Based on that, how many tables are there? */
        TableCount = (Rsdt->Header.Length - HeaderLength) / sizeof(ULONG);
    }

    /* Loop through the ACPI tables */
    for (i = 0; i < TableCount; i++)
    {
        /* For an XSDT, read the 64-bit address directly */
        if (Xsdt)
        {
            PhysicalAddress = Xsdt->Tables[i];
        }
        else
        {
            /* For RSDT, cast it */
            PhysicalAddress.QuadPart = Rsdt->Tables[i];
        }

        /* Map the header */
        Status = BlMmMapPhysicalAddressEx((PVOID)&Header,
                                          0,
                                          sizeof(*Header),
                                          PhysicalAddress);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Is it the right one? */
        if (Header->Signature == Signature)
        {
            /* Unmap the header */
            BlMmUnmapVirtualAddressEx(Header, sizeof(*Header));

            /* Map the whole table */
            return BlMmMapPhysicalAddressEx(TableAddress,
                                            0,
                                            Header->Length,
                                            PhysicalAddress);
        }
    }

    /* Requested table does not exist */
    return STATUS_NOT_FOUND;
}


VOID
BlUtlUpdateProgress (
    _In_ ULONG Percentage,
    _Out_opt_ PBOOLEAN Completed
    )
{
    if (UtlProgressRoutine)
    {
        EfiPrintf(L"Unimplemented\r\n");
    }
    else if (*Completed)
    {
        *Completed = TRUE;
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

VOID
BmUpdateProgressInfo (
    _In_ PVOID Unknown,
    _In_ PWCHAR ProgressInfo
    )
{
    EfiPrintf(L"Progress Info: %s\r\n", ProgressInfo);
}

VOID
BmUpdateProgress (
    _In_ PVOID Unknown,
    _In_ ULONG Percent,
    _Out_ PBOOLEAN Completed
    )
{
    EfiPrintf(L"Progress: %d\r\n", Percent);
    if (Completed)
    {
        *Completed = TRUE;
    }
}

NTSTATUS
BlUtlRegisterProgressRoutine (
    VOID
    )
{
    /* One shouldn't already exist */
    if (UtlProgressRoutine)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Set the routine, and no context */
    UtlProgressRoutine = BmUpdateProgress;
    UtlProgressContext = NULL;

    /* Progress increases by one */
    UtlProgressGranularity = 1;

    /* Set progress to zero for now */
    UtlCurrentPercentComplete = 0;
    UtlNextUpdatePercentage = 0;

    /* Set the info routine if there is one */
    UtlProgressInfoRoutine = BmUpdateProgressInfo;

    /* All good */
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
    ULONG Index;
    BOOLEAN Result;

    /* Check for invalid parameters */
    if (!(Table) || !(EntryIndex))
    {
        return Entry;
    }

    /* Loop each entry in the table */
    for (Index = 0; Index < Count;  Index++)
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
                /* Entry found return it */
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

    /* Check for invalid table ID, missing arguments, or malformed entry */
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

NTSTATUS
BlHtDelete (
    _In_ ULONG TableId,
    _In_ PBL_HASH_ENTRY Entry
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
        !(Entry->Size) ||
        !(Entry->Value) ||
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
                /* Remove it from the list and free it */
                RemoveEntryList(&HashNode->ListEntry);
                BlMmFreeHeap(HashNode);
                return STATUS_SUCCESS;
            }

            /* Try the next node */
            HashLink = HashLink->Flink;
        }
    }

    /* Return back to the caller */
    return Status;
}

ULONG
BlUtlCheckSum (
    _In_ ULONG PartialSum,
    _In_ PUCHAR Buffer,
    _In_ ULONG Length,
    _In_ ULONG Flags
    )
{
    ULONG i;

    if (Flags & BL_UTL_CHECKSUM_UCHAR_BUFFER)
    {
        EfiPrintf(L"Not supported\r\n");
        return 0;
    }
    else if (Flags & BL_UTL_CHECKSUM_USHORT_BUFFER)
    {
        PartialSum = (unsigned __int16)PartialSum;
        Length &= ~1;

        for (i = 0; i < Length; i += 2)
        {
            PartialSum += *(unsigned __int16 *)&Buffer[i];
            if (Flags & BL_UTL_CHECKSUM_COMPLEMENT)
            {
                PartialSum = (unsigned __int16)((PartialSum >> 16) + PartialSum);
            }
        }

        if (i != Length)
        {
            PartialSum += (unsigned __int8)Buffer[Length];
            if (Flags & BL_UTL_CHECKSUM_COMPLEMENT)
            {
                PartialSum = (unsigned __int16)((PartialSum >> 16) + PartialSum);
            }
        }

        if (Flags & BL_UTL_CHECKSUM_NEGATE)
        {
            return ~PartialSum;
        }

        PartialSum = (unsigned __int16)PartialSum;
    }
    else
    {
        /* Invalid mode */
        return 0;
    }

    if (Flags & BL_UTL_CHECKSUM_NEGATE)
    {
        return ~PartialSum;
    }

    return PartialSum;
}

#if defined(_M_IX86) || defined(_M_X64)
BOOLEAN
Archx86IsCpuidSupported (
    VOID
    )
{
    ULONG CallerFlags, Flags;

    /* Read the original flags, and add the CPUID bit */
    CallerFlags = __readeflags() ^ 0x200000;
    __writeeflags(CallerFlags);

    /* Read our flags now */
    Flags = __readeflags();

    /* Check if the bit stuck */
    return (((CallerFlags ^ Flags) >> 21) & 1) ^ 1;
}
#endif

BOOLEAN
BlArchIsCpuIdFunctionSupported (
    _In_ ULONG Function
    )
{
#if defined(_M_IX86) || defined(_M_X64)
    BOOLEAN Supported;
    INT CpuInfo[4];

    /* Check if the CPU supports this instruction */
    Supported = Archx86IsCpuidSupported();
    if (!Supported)
    {
        return FALSE;
    }

    /* Check if it's the extended function */
    if (Function >= 0x80000000)
    {
        /* Check if extended functions are supported */
        __cpuid(CpuInfo, 0x80000000);
        if ((CpuInfo[0] & 0xFFFFFF00) != 0x80000000)
        {
            /* Nope */
            return FALSE;
        }
    }
    else
    {
        /* It's a regular function, get the maximum one supported */
        __cpuid(CpuInfo, 0);
    }

    /* Check if our function is within bounds */
    if (Function <= CpuInfo[0])
    {
        return TRUE;
    }
#else
    EfiPrintf(L"BlArchIsCpuIdFunctionSupported not implemented for this platform.\r\n");
#endif

    /* Nope */
    return FALSE;
}

ULONGLONG
BlArchGetPerformanceCounter (
    VOID
    )
{
#if defined(_M_IX86) || defined(_M_X64)
    CPU_INFO CpuInfo;

    /* Serialize with CPUID, if it exists */
    if (Archx86IsCpuidSupported())
    {
        BlArchCpuId(0, 0, &CpuInfo);
    }

    /* Read the TSC */
    return __rdtsc();
#else
    EfiPrintf(L"BlArchGetPerformanceCounter not implemented for this platform.\r\n");
    return 0;
#endif
}

VOID
BlArchCpuId (
    _In_ ULONG Function,
    _In_ ULONG SubFunction,
    _Out_ PCPU_INFO Result
    )
{
#if defined(_M_IX86) || defined(_M_X64)
    /* Use the intrinsic */
    __cpuidex((INT*)Result->AsUINT32, Function, SubFunction);
#endif
}

CPU_VENDORS
BlArchGetCpuVendor (
    VOID
    )
{
    CPU_INFO CpuInfo;
    INT Temp;

    /* Get the CPU Vendor */
    BlArchCpuId(0, 0, &CpuInfo);
#if defined(_M_IX86) || defined(_M_X64)
    Temp = CpuInfo.Ecx;
    CpuInfo.Ecx = CpuInfo.Edx;
    CpuInfo.Edx = Temp;

    /* Check against supported values */
    if (!strncmp((PCHAR)&CpuInfo.Ebx, "GenuineIntel", 12))
    {
        return CPU_INTEL;
    }
    if (!strncmp((PCHAR)&CpuInfo.Ebx, "AuthenticAMD", 12))
    {
        return CPU_AMD;
    }
    if (!strncmp((PCHAR)&CpuInfo.Ebx, "CentaurHauls", 12))
    {
        return CPU_VIA;
    }
#ifdef _M_IX86
    if (!strncmp((PCHAR)&CpuInfo.Ebx, "CyrixInstead", 12))
    {
        return CPU_CYRIX;
    }
    if (!strncmp((PCHAR)&CpuInfo.Ebx, "GenuineTMx86", 12))
    {
        return CPU_TRANSMETA;
    }
    if (!strncmp((PCHAR)&CpuInfo.Ebx, "RiseRiseRise", 12))
    {
        return CPU_RISE;
    }
#endif // _M_IX86
#else // defined(_M_IX86) || defined(_M_X64)
    EfiPrintf(L"BlArchGetCpuVendor not implemented for this platform.\r\n");
#endif
    /* Other */
    return CPU_UNKNOWN;
}

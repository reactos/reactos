/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/wmi/smbios.c
 * PURPOSE:         I/O Windows Management Instrumentation (WMI) Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <wmiguid.h>
#include <wmidata.h>
#include <wmistr.h>

#include "wmip.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

typedef struct _SMBIOS21_ENTRY_POINT
{
 	CHAR AnchorString[4];
 	UCHAR Checksum;
 	UCHAR Length;
 	UCHAR MajorVersion;
 	UCHAR MinorVersion;
 	USHORT MaxStructureSize;
 	UCHAR EntryPointRevision;
 	CHAR FormattedArea[5];
 	CHAR AnchorString2[5];
 	UCHAR Checksum2;
 	USHORT TableLength;
 	ULONG TableAddress;
 	USHORT NumberOfStructures;
 	UCHAR BCDRevision;
} SMBIOS21_ENTRY_POINT, *PSMBIOS21_ENTRY_POINT;

typedef struct _SMBIOS30_ENTRY_POINT
{
 	CHAR AnchorString[5];
 	UCHAR Checksum;
 	UCHAR Length;
 	UCHAR MajorVersion;
 	UCHAR MinorVersion;
 	UCHAR Docref;
    UCHAR Revision;
    UCHAR Reserved;
    ULONG TableMaxSize;
    ULONG64 TableAddress;
} SMBIOS30_ENTRY_POINT, *PSMBIOS30_ENTRY_POINT;

static
BOOLEAN
GetEntryPointData(
    _In_ const UCHAR *EntryPointAddress,
    _Out_ PULONG64 TableAddress,
    _Out_ PULONG TableSize,
    _Out_ PMSSmBios_RawSMBiosTables BiosTablesHeader)
{
    PSMBIOS21_ENTRY_POINT EntryPoint21;
    PSMBIOS30_ENTRY_POINT EntryPoint30;
    UCHAR Checksum;
    ULONG i;

    /* Check for SMBIOS 2.1 entry point */
    EntryPoint21 = (PSMBIOS21_ENTRY_POINT)EntryPointAddress;
    if (RtlEqualMemory(EntryPoint21->AnchorString, "_SM_", 4))
    {
        if (EntryPoint21->Length > 32)
            return FALSE;

        /* Calculate the checksum */
        Checksum = 0;
        for (i = 0; i < EntryPoint21->Length; i++)
        {
            Checksum += EntryPointAddress[i];
        }

        if (Checksum != 0)
            return FALSE;

        *TableAddress = EntryPoint21->TableAddress;
        *TableSize = EntryPoint21->TableLength;
        BiosTablesHeader->Used20CallingMethod = 0;
        BiosTablesHeader->SmbiosMajorVersion = EntryPoint21->MajorVersion;
        BiosTablesHeader->SmbiosMinorVersion = EntryPoint21->MinorVersion;
        BiosTablesHeader->DmiRevision = 2;
        BiosTablesHeader->Size = EntryPoint21->TableLength;
        return TRUE;
    }

    /* Check for SMBIOS 3.0 entry point */
    EntryPoint30 = (PSMBIOS30_ENTRY_POINT)EntryPointAddress;
    if (RtlEqualMemory(EntryPoint30->AnchorString, "_SM3_", 5))
    {
        if (EntryPoint30->Length > 32)
            return FALSE;

        /* Calculate the checksum */
        Checksum = 0;
        for (i = 0; i < EntryPoint30->Length; i++)
        {
            Checksum += EntryPointAddress[i];
        }

        if (Checksum != 0)
            return FALSE;

        *TableAddress = EntryPoint30->TableAddress;
        *TableSize = EntryPoint30->TableMaxSize;
        BiosTablesHeader->Used20CallingMethod = 0;
        BiosTablesHeader->SmbiosMajorVersion = EntryPoint30->MajorVersion;
        BiosTablesHeader->SmbiosMinorVersion = EntryPoint30->MinorVersion;
        BiosTablesHeader->DmiRevision = 3;
        BiosTablesHeader->Size = EntryPoint30->TableMaxSize;
        return TRUE;
    }

    return FALSE;
}

_At_(*OutTableData, __drv_allocatesMem(Mem))
NTSTATUS
NTAPI
WmipGetRawSMBiosTableData(
    _Outptr_opt_result_buffer_(*OutDataSize) PVOID *OutTableData,
    _Out_ PULONG OutDataSize)
{
    static const SIZE_T SearchSize = 0x10000;
    static const ULONG HeaderSize = FIELD_OFFSET(MSSmBios_RawSMBiosTables, SMBiosData);
    PHYSICAL_ADDRESS PhysicalAddress;
    PUCHAR EntryPointMapping;
    MSSmBios_RawSMBiosTables BiosTablesHeader;
    PVOID BiosTables, TableMapping;
    ULONG Offset, TableSize;
    ULONG64 TableAddress = 0;

    /* This is where the range for the entry point starts */
    PhysicalAddress.QuadPart = 0xF0000;

    /* Map the range into the system address space */
    EntryPointMapping = MmMapIoSpace(PhysicalAddress, SearchSize, MmCached);
    if (EntryPointMapping == NULL)
    {
        DPRINT1("Failed to map range for SMBIOS entry point\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Loop the table memory in 16 byte steps */
    for (Offset = 0; Offset <= (0x10000 - 32); Offset += 16)
    {
        /* Check if we have an entry point here and get it's data */
        if (GetEntryPointData(EntryPointMapping + Offset,
                              &TableAddress,
                              &TableSize,
                              &BiosTablesHeader))
        {
            break;
        }
    }

    /* Unmap the entry point */
    MmUnmapIoSpace(EntryPointMapping, SearchSize);

    /* Did we find anything */
    if (TableAddress == 0)
    {
        DPRINT1("Could not find the SMBIOS entry point\n");
        return STATUS_NOT_FOUND;
    }

    /* Check if the caller asked for the buffer */
    if (OutTableData != NULL)
    {
        /* Allocate a buffer for the result */
        BiosTables = ExAllocatePoolWithTag(PagedPool,
                                           HeaderSize + TableSize,
                                           'BTMS');
        if (BiosTables == NULL)
        {
            DPRINT1("Failed to allocate %lu bytes for the SMBIOS table\n",
                    HeaderSize + TableSize);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Copy the header */
        RtlCopyMemory(BiosTables, &BiosTablesHeader, HeaderSize);

        /* This is where the table is */
        PhysicalAddress.QuadPart = TableAddress;

        /* Map the table into the system address space */
        TableMapping = MmMapIoSpace(PhysicalAddress, TableSize, MmCached);
        if (TableMapping == NULL)
        {
            ExFreePoolWithTag(BiosTables, 'BTMS');
            return STATUS_UNSUCCESSFUL;
        }

        /* Copy the table */
        RtlCopyMemory((PUCHAR)BiosTables + HeaderSize, TableMapping, TableSize);

        /* Unmap the table */
        MmUnmapIoSpace(TableMapping, TableSize);

        *OutTableData = BiosTables;
    }

    *OutDataSize = HeaderSize + TableSize;
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
WmipQueryRawSMBiosTables(
    _Inout_ ULONG *InOutBufferSize,
    _Out_opt_ PVOID OutBuffer)
{
    NTSTATUS Status;
    PVOID TableData = NULL;
    ULONG TableSize, ResultSize;
    PWNODE_ALL_DATA AllData;

    /* Get the table data */
    Status = WmipGetRawSMBiosTableData(OutBuffer ? &TableData : NULL, &TableSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WmipGetRawSMBiosTableData failed: 0x%08lx\n", Status);
        return Status;
    }

    ResultSize = sizeof(WNODE_ALL_DATA) + TableSize;

    /* Check if the caller provided a buffer */
    if ((OutBuffer != NULL) && (*InOutBufferSize != 0))
    {
        /* Check if the buffer is large enough */
        if (*InOutBufferSize < ResultSize)
        {
            DPRINT1("Buffer too small. Got %lu, need %lu\n",
                    *InOutBufferSize, ResultSize);
            return STATUS_BUFFER_TOO_SMALL;
        }

        /// FIXME: most of this is fubar
        AllData = OutBuffer;
        AllData->WnodeHeader.BufferSize = ResultSize;
        AllData->WnodeHeader.ProviderId = 0;
        AllData->WnodeHeader.Version = 0;
        AllData->WnodeHeader.Linkage = 0; // last entry
        //AllData->WnodeHeader.CountLost;
        AllData->WnodeHeader.KernelHandle = NULL;
        //AllData->WnodeHeader.TimeStamp;
        AllData->WnodeHeader.Guid = MSSmBios_RawSMBiosTables_GUID;
        //AllData->WnodeHeader.ClientContext;
        AllData->WnodeHeader.Flags = WNODE_FLAG_FIXED_INSTANCE_SIZE;
        AllData->DataBlockOffset = sizeof(WNODE_ALL_DATA);
        AllData->InstanceCount = 1;
        //AllData->OffsetInstanceNameOffsets;
        AllData->FixedInstanceSize = TableSize;

        RtlCopyMemory(AllData + 1, TableData, TableSize);
    }

    /* Set the size */
    *InOutBufferSize = ResultSize;

    /* Free the table buffer */
    if (TableData != NULL)
    {
        ExFreePoolWithTag(TableData, 'BTMS');
    }

    return STATUS_SUCCESS;
}


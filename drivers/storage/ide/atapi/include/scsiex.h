/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Extra SCSI definitions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <pshpack1.h>

typedef union _SCSI_SENSE_CODE
{
    struct
    {
        UCHAR SrbStatus;
        UCHAR SenseKey;
        UCHAR AdditionalSenseCode;
        UCHAR AdditionalSenseCodeQualifier;
    };
    ULONG AsULONG;
} SCSI_SENSE_CODE, *PSCSI_SENSE_CODE;

typedef struct _SCSI_SENSE_KEY_SPECIFIC_FIELD_POINTER
{
    UCHAR BitPointer:3;
    UCHAR BitPointerValid:1;
    UCHAR Reserved:2;
    UCHAR CommandData:1;
    UCHAR SenseKeySpecificValid:1;
    UCHAR FieldPointer[2];
} SCSI_SENSE_KEY_SPECIFIC_FIELD_POINTER, *PSCSI_SENSE_KEY_SPECIFIC_FIELD_POINTER;
// CommandData = 1
// SenseKeySpecificValid = 1

C_ASSERT(sizeof(SCSI_SENSE_KEY_SPECIFIC_FIELD_POINTER) == 3);

typedef struct _MODE_CACHING_PAGE_SPC5
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR ReadDisableCache:1;
    UCHAR MultiplicationFactor:1;
    UCHAR WriteCacheEnable:1;
    UCHAR Reserved2:5;
    UCHAR WriteRetensionPriority:4;
    UCHAR ReadRetensionPriority:4;
    UCHAR DisablePrefetchTransfer[2];
    UCHAR MinimumPrefetch[2];
    UCHAR MaximumPrefetch[2];
    UCHAR MaximumPrefetchCeiling[2];
    UCHAR NV_DIS:1;
    UCHAR SYNC_PROG:2;
    UCHAR Reserved1:2;
    UCHAR DisableReadAHead:1;
    UCHAR LBCSS:1;
    UCHAR FSW:1;
    UCHAR NumberOfCacheSegments;
    UCHAR CacheSegmentSize[2];
    UCHAR Reserved3;
    UCHAR Obsolete[3];
} MODE_CACHING_PAGE_SPC5, *PMODE_CACHING_PAGE_SPC5;

C_ASSERT(sizeof(MODE_CACHING_PAGE_SPC5) == 20);

typedef struct _MODE_CONTROL_EXTENSION_PAGE
{
    UCHAR PageCode:6;
    UCHAR SubPageFormat:1;
    UCHAR PageSavable:1;
    UCHAR SubPageCode;
    UCHAR PageLength[2];
    UCHAR IALUAE:1;
    UCHAR SCSIP:1;
    UCHAR TCMOS:1;
    UCHAR Reserved:5;
    UCHAR InitialCommandPriority:4;
    UCHAR Reserved1:4;
    UCHAR MaximumSenseDataLength;
    UCHAR Reserved2[25];
} MODE_CONTROL_EXTENSION_PAGE, *PMODE_CONTROL_EXTENSION_PAGE;

C_ASSERT(sizeof(MODE_CONTROL_EXTENSION_PAGE) == 32);

#include <poppack.h>

#define SCSI_ADSENSE_ADDRESS_MARK_NOT_FOUND_FOR_DATA_FIELD  0x13

/* SAT-6 */
C_ASSERT(sizeof(MODE_INFO_EXCEPTIONS) == 12);
C_ASSERT(sizeof(MODE_CONTROL_PAGE) == 12);
C_ASSERT(sizeof(MODE_READ_WRITE_RECOVERY_PAGE) == 12);
C_ASSERT(sizeof(POWER_CONDITION_PAGE) == 12);
C_ASSERT(sizeof(VPD_ATA_INFORMATION_PAGE) == 572);
C_ASSERT(sizeof(VPD_BLOCK_LIMITS_PAGE) == 0x3c+4);
C_ASSERT(sizeof(VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE) == 0x3c+4);

FORCEINLINE
UCHAR
CdbGetAllocationLength6(
    _In_ PCDB Cdb)
{
    return Cdb->CDB6GENERIC.CommandUniqueBytes[2];
}

FORCEINLINE
USHORT
CdbGetAllocationLength10(
    _In_ PCDB Cdb)
{
    return (Cdb->CDB10.TransferBlocksMsb << 8) |
           (Cdb->CDB10.TransferBlocksLsb << 0);
}

FORCEINLINE
ULONG
CdbGetAllocationLength16(
    _In_ PCDB Cdb)
{
    return (Cdb->CDB16.TransferLength[0] << 24) |
           (Cdb->CDB16.TransferLength[1] << 16) |
           (Cdb->CDB16.TransferLength[2] << 8) |
           (Cdb->CDB16.TransferLength[3] << 0);
}

FORCEINLINE
USHORT
CdbGetTransferLength10(
    _In_ PCDB Cdb)
{
    /* Bytes 7:8 */
    return (Cdb->CDB10.TransferBlocksMsb << 8) |
           (Cdb->CDB10.TransferBlocksLsb << 0);
}

FORCEINLINE
ULONG
CdbGetTransferLength12(
    _In_ PCDB Cdb)
{
    /* Bytes 6:9 */
    return (Cdb->CDB12.TransferLength[0] << 24) |
           (Cdb->CDB12.TransferLength[1] << 16) |
           (Cdb->CDB12.TransferLength[2] << 8) |
           (Cdb->CDB12.TransferLength[3] << 0);
}

FORCEINLINE
ULONG
CdbGetTransferLength16(
    _In_ PCDB Cdb)
{
    /* Bytes 10:13 */
    return (Cdb->CDB16.TransferLength[0] << 24) |
           (Cdb->CDB16.TransferLength[1] << 16) |
           (Cdb->CDB16.TransferLength[2] << 8) |
           (Cdb->CDB16.TransferLength[3] << 0);
}

FORCEINLINE
ULONG
CdbGetLogicalBlockAddress6(
    _In_ PCDB Cdb)
{
    /* Bytes 2:3 */
    return (Cdb->CDB6READWRITE.LogicalBlockMsb0 << 8) |
           (Cdb->CDB6READWRITE.LogicalBlockLsb << 0);
}

FORCEINLINE
ULONG
CdbGetLogicalBlockAddress10(
    _In_ PCDB Cdb)
{
    /* Bytes 2:5 */
    return (Cdb->CDB10.LogicalBlockByte0 << 24) |
           (Cdb->CDB10.LogicalBlockByte1 << 16) |
           (Cdb->CDB10.LogicalBlockByte2 << 8) |
           (Cdb->CDB10.LogicalBlockByte3 << 0);
}

FORCEINLINE
ULONG
CdbGetLogicalBlockAddress12(
    _In_ PCDB Cdb)
{
    /* Bytes 2:5 */
    return (Cdb->CDB12.LogicalBlock[0] << 24) |
           (Cdb->CDB12.LogicalBlock[1] << 16) |
           (Cdb->CDB12.LogicalBlock[2] << 8) |
           (Cdb->CDB12.LogicalBlock[3] << 0);
}

FORCEINLINE
ULONG64
CdbGetLogicalBlockAddress16(
    _In_ PCDB Cdb)
{
    /* Bytes 2:9 */
    return ((ULONG64)Cdb->CDB16.LogicalBlock[0] << 56) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[1] << 48) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[2] << 40) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[3] << 32) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[4] << 24) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[5] << 16) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[6] << 8) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[7] << 0);
}

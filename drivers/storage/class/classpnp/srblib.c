/*++

Copyright (C) Microsoft Corporation 2010

Module Name:

    srblib.c

Abstract:

    Header for SRB utility functions

Environment:

    kernel mode only

Notes:


Revision History:

--*/


#include "classp.h"

PVOID
DefaultStorageRequestBlockAllocateRoutine(
    _In_ CLONG ByteSize
    )
/*++

Routine Description:

    Default allocation routine.

Arguments:

    ByteSize - SRB size in bytes.

Return Value:

    Pointer to the SRB buffer. NULL if SRB buffer could not be allocated.

--*/
{
    return ExAllocatePoolWithTag(NonPagedPoolNx, ByteSize, '+brs');
}


NTSTATUS
pInitializeStorageRequestBlock(
    _Inout_bytecount_(ByteSize) PSTORAGE_REQUEST_BLOCK Srb,
    _In_ USHORT AddressType,
    _In_ ULONG ByteSize,
    _In_ ULONG NumSrbExData,
    _In_ va_list ap
    )
/*++

Routine Description:

    Initialize a STORAGE_REQUEST_BLOCK.

Arguments:

    Srb - Pointer to STORAGE_REQUEST_BLOCK to initialize.

    AddressType - Storage address type.

    ByteSize - STORAGE_REQUEST_BLOCK size in bytes.

    NumSrbExData - Number of SRB extended data.

    ap - Variable argument list matching the SRB extended data in the
         STORAGE_REQUEST_BLOCK.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTOR_ADDRESS address;
    PSRBEX_DATA srbExData;
    ULONG offset;
    ULONG length = (ULONG)-1;
    SRBEXDATATYPE type;
    ULONG srbExDataLength = (ULONG)-1;
    ULONG varLength;
    ULONG i;

    if (ByteSize < sizeof(STORAGE_REQUEST_BLOCK)) {
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory(Srb, ByteSize);

    Srb->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
    Srb->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
    Srb->Signature = SRB_SIGNATURE;
    Srb->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
    Srb->SrbLength = ByteSize;
    Srb->NumSrbExData = NumSrbExData;

    offset = sizeof(STORAGE_REQUEST_BLOCK);
    if (NumSrbExData > 0) {
        offset += ((NumSrbExData - 1) * sizeof(ULONG));

        // Ensure offset is pointer type aligned
        if (offset % sizeof(PVOID)) {
            offset += (sizeof(PVOID) - (offset % sizeof(PVOID)));
        }
    }
    Srb->AddressOffset = offset;

    if (AddressType == STORAGE_ADDRESS_TYPE_BTL8)
    {
        if ((ByteSize < offset) ||
            (ByteSize < (offset + sizeof(STOR_ADDR_BTL8)))) {
            return STATUS_BUFFER_OVERFLOW;
        }
        address = (PSTOR_ADDRESS)((PUCHAR)Srb + offset);
        address->Type = STOR_ADDRESS_TYPE_BTL8;
        address->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;
        offset += sizeof(STOR_ADDR_BTL8);
    } else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < NumSrbExData && status == STATUS_SUCCESS; i++)
    {
        if (ByteSize <= offset) {
            status = STATUS_BUFFER_OVERFLOW;
            break;
        }
        srbExData = (PSRBEX_DATA)((PUCHAR)Srb + offset);
        Srb->SrbExDataOffset[i] = offset;

        type = va_arg(ap, SRBEXDATATYPE);

        switch (type)
        {
            case SrbExDataTypeBidirectional:
                length = sizeof(SRBEX_DATA_BIDIRECTIONAL);
                srbExDataLength = SRBEX_DATA_BIDIRECTIONAL_LENGTH;
                break;
            case SrbExDataTypeScsiCdb16:
                length = sizeof(SRBEX_DATA_SCSI_CDB16);
                srbExDataLength = SRBEX_DATA_SCSI_CDB16_LENGTH;
                break;
            case SrbExDataTypeScsiCdb32:
                length = sizeof(SRBEX_DATA_SCSI_CDB32);
                srbExDataLength = SRBEX_DATA_SCSI_CDB32_LENGTH;
                break;
            case SrbExDataTypeScsiCdbVar:
                varLength = va_arg(ap, ULONG);
                length = sizeof(SRBEX_DATA_SCSI_CDB_VAR) + varLength;
                srbExDataLength = SRBEX_DATA_SCSI_CDB_VAR_LENGTH_MIN + varLength;
                break;
            case SrbExDataTypeWmi:
                length = sizeof(SRBEX_DATA_WMI);
                srbExDataLength = SRBEX_DATA_WMI_LENGTH;
                break;
            case SrbExDataTypePower:
                length = sizeof(SRBEX_DATA_POWER);
                srbExDataLength = SRBEX_DATA_POWER_LENGTH;
                break;
            case SrbExDataTypePnP:
                length = sizeof(SRBEX_DATA_PNP);
                srbExDataLength = SRBEX_DATA_PNP_LENGTH;
                break;
            case SrbExDataTypeIoInfo:
                length = sizeof(SRBEX_DATA_IO_INFO);
                srbExDataLength = SRBEX_DATA_IO_INFO_LENGTH;
                break;
            default:
                status = STATUS_INVALID_PARAMETER;
                break;
        }

        if (status == STATUS_SUCCESS)
        {
            NT_ASSERT(length != (ULONG)-1);

            if (ByteSize < (offset + length)) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            NT_ASSERT(srbExDataLength != (ULONG)-1);

            srbExData->Type = type;
            srbExData->Length = srbExDataLength;
            offset += length;
        }
    }

    return status;
}


NTSTATUS
InitializeStorageRequestBlock(
    _Inout_bytecount_(ByteSize) PSTORAGE_REQUEST_BLOCK Srb,
    _In_ USHORT AddressType,
    _In_ ULONG ByteSize,
    _In_ ULONG NumSrbExData,
    ...
    )
/*++

Routine Description:

    Initialize an extended SRB.

Arguments:

    Srb - Pointer to SRB buffer to initialize.

    AddressType - Storage address type.

    ByteSize - STORAGE_REQUEST_BLOCK size in bytes.

    NumSrbExData - Number of SRB extended data.

    ... - Variable argument list matching the SRB extended data in the
          STORAGE_REQUEST_BLOCK.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    va_list ap;
    va_start(ap, NumSrbExData);
    status = pInitializeStorageRequestBlock(Srb, AddressType, ByteSize, NumSrbExData, ap);
    va_end(ap);
    return status;
}



NTSTATUS
CreateStorageRequestBlock(
    _Inout_ PSTORAGE_REQUEST_BLOCK *Srb,
    _In_ USHORT AddressType,
    _In_opt_ PSRB_ALLOCATE_ROUTINE AllocateRoutine,
    _Inout_opt_ ULONG *ByteSize,
    _In_ ULONG NumSrbExData,
    ...
    )
/*++

Routine Description:

    Create an extended SRB.

Arguments:

    Srb - Pointer to buffer to store SRB pointer.

    AddressType - Storage address type.

    AllocateRoutine - Buffer allocation function (optional).

    ByteSize - Pointer to ULONG to store size of SRB in bytes (optional).

    NumSrbExData - Number of SRB extended data.

    ... - Variable argument list matching the SRB extended data in the
          STORAGE_REQUEST_BLOCK.

Return Value:

    NTSTATUS

--*/
{
    ULONG sizeNeeded = 0;
    va_list ap;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    // Ensure SrbExData offsets are pointer type aligned
    sizeNeeded = sizeof(STORAGE_REQUEST_BLOCK);
    if (NumSrbExData > 0) {
        sizeNeeded += ((NumSrbExData - 1) * sizeof(ULONG));
        if (sizeNeeded % sizeof(PVOID)) {
            sizeNeeded += (sizeof(PVOID) - (sizeNeeded % sizeof(PVOID)));
        }
    }

    if (AddressType == STORAGE_ADDRESS_TYPE_BTL8)
    {
        sizeNeeded += sizeof(STOR_ADDR_BTL8);
    } else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    va_start(ap, NumSrbExData);

    for (i = 0; i < NumSrbExData && status == STATUS_SUCCESS; i++)
    {
        switch (va_arg(ap, SRBEXDATATYPE))
        {
            case SrbExDataTypeBidirectional:
                sizeNeeded += sizeof(SRBEX_DATA_BIDIRECTIONAL);
                break;
            case SrbExDataTypeScsiCdb16:
                sizeNeeded += sizeof(SRBEX_DATA_SCSI_CDB16);
                break;
            case SrbExDataTypeScsiCdb32:
                sizeNeeded += sizeof(SRBEX_DATA_SCSI_CDB32);
                break;
            case SrbExDataTypeScsiCdbVar:
                sizeNeeded += sizeof(SRBEX_DATA_SCSI_CDB_VAR) + va_arg(ap, ULONG);
                break;
            case SrbExDataTypeWmi:
                sizeNeeded += sizeof(SRBEX_DATA_WMI);
                break;
            case SrbExDataTypePower:
                sizeNeeded += sizeof(SRBEX_DATA_POWER);
                break;
            case SrbExDataTypePnP:
                sizeNeeded += sizeof(SRBEX_DATA_PNP);
                break;
            case SrbExDataTypeIoInfo:
                sizeNeeded += sizeof(SRBEX_DATA_IO_INFO);
                break;
            default:
                status = STATUS_INVALID_PARAMETER;
                break;
        }
    }
    va_end(ap);

    if (status == STATUS_SUCCESS)
    {
        if (AllocateRoutine)
        {
            *Srb = AllocateRoutine(sizeNeeded);
            if (*Srb == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (ByteSize != NULL)
        {
            *ByteSize = sizeNeeded;
        }

        if (*Srb)
        {
            va_start(ap, NumSrbExData);
#ifdef _MSC_VER
            #pragma prefast(suppress:26015, "pInitializeStorageRequestBlock will set the SrbLength field")
#endif
            status = pInitializeStorageRequestBlock(*Srb, AddressType, sizeNeeded, NumSrbExData, ap);
            va_end(ap);
        }

    }

    return status;
}





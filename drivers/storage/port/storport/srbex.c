/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport extended request block translation
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* The extended request trails the request reference in the same allocation */
C_ASSERT((sizeof(QUEUED_REQUEST_REFERENCE) % TYPE_ALIGNMENT(EXTENDED_REQUEST)) == 0);

/* A miniport walks to the address and the command block by offset */
C_ASSERT(FIELD_OFFSET(EXTENDED_REQUEST, Address) >= sizeof(STORAGE_REQUEST_BLOCK));
C_ASSERT(FIELD_OFFSET(EXTENDED_REQUEST, Cdb) >=
         FIELD_OFFSET(EXTENDED_REQUEST, Address) + sizeof(STOR_ADDR_BTL8));


/* FUNCTIONS ******************************************************************/

/**
 * @brief Fills in the bus, target and LUN address of a logical unit.
 */
VOID
NTAPI
StorpBuildUnitAddress(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _Out_ PSTOR_ADDR_BTL8 Address)
{
    RtlZeroMemory(Address, sizeof(*Address));

    Address->Type = STOR_ADDRESS_TYPE_BTL8;
    Address->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;
    Address->Path = (UCHAR)PdoExtension->Bus;
    Address->Target = (UCHAR)PdoExtension->Target;
    Address->Lun = (UCHAR)PdoExtension->Lun;
}


/**
 * @brief Tells whether a request block is in the extended format.
 */
BOOLEAN
NTAPI
StorpIsExtendedSrb(
    _In_ PVOID Srb)
{
    PSTORAGE_REQUEST_BLOCK_HEADER Header = (PSTORAGE_REQUEST_BLOCK_HEADER)Srb;

    return (Header->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK);
}


/**
 * @brief Fills in the extended request a miniport will be given.
 *
 * @param Srb The request as the class layer built it.
 * @param Request Storage for the translated request, already zeroed.
 */
VOID
NTAPI
StorpBuildExtendedSrb(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PEXTENDED_REQUEST Request)
{
    PSTORAGE_REQUEST_BLOCK Extended = &Request->Srb;

    Extended->Length = sizeof(*Extended);
    Extended->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
    Extended->SrbStatus = SRB_STATUS_PENDING;
    Extended->Signature = SRB_SIGNATURE;
    Extended->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
    Extended->SrbLength = sizeof(*Request);
    Extended->SrbFunction = Srb->Function;
    Extended->SrbFlags = Srb->SrbFlags;
    Extended->RequestTag = Srb->QueueTag;
    Extended->RequestAttribute = Srb->QueueAction;
    Extended->TimeOutValue = Srb->TimeOutValue;
    Extended->DataTransferLength = Srb->DataTransferLength;
    Extended->DataBuffer = Srb->DataBuffer;

    /* The miniport hands this straight back to us on completion */
    Extended->OriginalRequest = Srb->OriginalRequest;
    Extended->MiniportContext = Srb->SrbExtension;

    Extended->AddressOffset = FIELD_OFFSET(EXTENDED_REQUEST, Address);

    Request->Address.Type = STOR_ADDRESS_TYPE_BTL8;
    Request->Address.AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;
    Request->Address.Path = Srb->PathId;
    Request->Address.Target = Srb->TargetId;
    Request->Address.Lun = Srb->Lun;

    /* Only SRB_FUNCTION_EXECUTE_SCSI carries a CDB */
    if (Srb->Function != SRB_FUNCTION_EXECUTE_SCSI)
        return;

    Extended->NumSrbExData = 1;
    Extended->SrbExDataOffset[0] = FIELD_OFFSET(EXTENDED_REQUEST, Cdb);

    Request->Cdb.Type = SrbExDataTypeScsiCdb16;
    Request->Cdb.Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
    Request->Cdb.ScsiStatus = Srb->ScsiStatus;
    Request->Cdb.SenseInfoBufferLength = Srb->SenseInfoBufferLength;
    Request->Cdb.CdbLength = Srb->CdbLength;
    Request->Cdb.SenseInfoBuffer = Srb->SenseInfoBuffer;

    RtlCopyMemory(Request->Cdb.Cdb, Srb->Cdb, sizeof(Request->Cdb.Cdb));
}


/**
 * @brief Carries the miniport's answer back to the original request block.
 */
VOID
NTAPI
StorpCompleteExtendedSrb(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PEXTENDED_REQUEST Request)
{
    PSTORAGE_REQUEST_BLOCK Extended = &Request->Srb;

    Srb->SrbStatus = Extended->SrbStatus;
    Srb->DataTransferLength = Extended->DataTransferLength;

    if (Extended->NumSrbExData == 0)
        return;

    Srb->ScsiStatus = Request->Cdb.ScsiStatus;
    Srb->SenseInfoBufferLength = Request->Cdb.SenseInfoBufferLength;
}

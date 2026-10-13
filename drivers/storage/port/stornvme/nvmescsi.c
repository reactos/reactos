/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     SCSI command translation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

static const UCHAR NvmpHexDigits[] = "0123456789ABCDEF";

/* Standard INQUIRY data is cut off after the version descriptors */
#define NVME_INQUIRY_DATA_LENGTH    60

/* Fixed format sense data, without the sense key specific bytes past byte 17 */
#define NVME_SENSE_DATA_LENGTH      18

/* Caching mode page as returned, and the shortest form a caller may ask for */
#define NVME_CACHING_PAGE_LENGTH    20
#define NVME_CACHING_PAGE_MINIMUM   12

/* SMART / Health Information log page */
#define NVME_HEALTH_LOG_LENGTH      512


/* FUNCTIONS ******************************************************************/

/**
 * @brief Returns the active namespace for a LUN, which is NSID - 1.
 */
PNVME_NAMESPACE
NvmpNamespaceFromLun(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG Lun)
{
    PNVME_NAMESPACE Namespace;

    if (Lun >= Adapter->NamespaceCount)
        return NULL;

    Namespace = &Adapter->Namespaces[Lun];
    if (!Namespace->Present)
        return NULL;

    return Namespace;
}


/**
 * @brief Writes fixed format sense data and the SCSI status.
 *
 * As much of the sense data as fits is copied, and any sense buffer at all marks it valid.
 */
VOID
NvmpSetSenseData(
    _In_ PVOID Srb,
    _In_ UCHAR ScsiStatus,
    _In_ UCHAR SenseKey,
    _In_ UCHAR AdditionalSenseCode,
    _In_ UCHAR AdditionalSenseCodeQualifier)
{
    SENSE_DATA Sense;
    PUCHAR Buffer;
    UCHAR Length;
    UCHAR Copied;

    RtlZeroMemory(&Sense, sizeof(Sense));
    Sense.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
    Sense.Valid = 1;
    Sense.SenseKey = SenseKey;
    Sense.AdditionalSenseLength = NVME_SENSE_DATA_LENGTH -
                                  FIELD_OFFSET(SENSE_DATA, AdditionalSenseLength) - 1;
    Sense.AdditionalSenseCode = AdditionalSenseCode;
    Sense.AdditionalSenseCodeQualifier = AdditionalSenseCodeQualifier;

    SrbSetScsiStatus(Srb, ScsiStatus);

    Buffer = SrbGetSenseInfoBuffer(Srb);
    Length = SrbGetSenseInfoBufferLength(Srb);
    if (Buffer == NULL || Length == 0)
        return;

    Copied = min(Length, NVME_SENSE_DATA_LENGTH);
    RtlCopyMemory(Buffer, &Sense, Copied);
    RtlZeroMemory(Buffer + Copied, Length - Copied);

    SrbSetSrbStatus(Srb, SrbGetSrbStatus(Srb) | SRB_STATUS_AUTOSENSE_VALID);
}


/**
 * @brief Completes a request with ILLEGAL REQUEST sense data.
 */
VOID
NvmpCompleteIllegalRequest(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ UCHAR SrbStatus,
    _In_ UCHAR AdditionalSenseCode)
{
    SrbSetSrbStatus(Srb, SrbStatus);
    NvmpSetSenseData(Srb, SCSISTAT_CHECK_CONDITION, SCSI_SENSE_ILLEGAL_REQUEST,
                     AdditionalSenseCode, 0);

    StorPortNotification(RequestComplete, Adapter, Srb);
}


/**
 * @brief Writes Value as hex digits, most significant first.
 */
static
VOID
NvmpWriteHex(
    _Out_writes_(Digits) PUCHAR Target,
    _In_ ULONG Value,
    _In_ ULONG Digits)
{
    while (Digits != 0)
    {
        Digits--;
        Target[Digits] = NvmpHexDigits[Value & 0xF];
        Value >>= 4;
    }
}


/**
 * @brief Returns TRUE if every byte of an identifier is zero.
 */
static
BOOLEAN
NvmpIsZero(
    _In_reads_bytes_(Length) const UCHAR *Data,
    _In_ ULONG Length)
{
    ULONG Index;

    for (Index = 0; Index < Length; Index++)
    {
        if (Data[Index] != 0)
            return FALSE;
    }

    return TRUE;
}


/**
 * @brief Copies the last Length printable, non blank characters of the firmware revision.
 *
 * Bytes of Target that are not filled keep what they held.
 */
static
VOID
NvmpCopyFirmwareRevision(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _Out_writes_bytes_(Length) PUCHAR Target,
    _In_ ULONG Length)
{
    const UCHAR *Revision = Adapter->FirmwareRevision;
    ULONG Start = sizeof(Adapter->FirmwareRevision);
    ULONG Found = 0;
    ULONG Index;

    /* Walk back to the first of the characters that will be kept */
    while (Start != 0 && Found < Length)
    {
        Start--;
        if (Revision[Start] > ' ' && Revision[Start] < 0x7F)
            Found++;
    }

    Found = 0;
    for (Index = Start; Index < sizeof(Adapter->FirmwareRevision) && Found < Length; Index++)
    {
        if (Revision[Index] > ' ' && Revision[Index] < 0x7F)
            Target[Found++] = Revision[Index];
    }
}


/**
 * @brief Returns standard INQUIRY data.
 */
static
VOID
NvmpInquiryStandard(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
    INQUIRYDATA Inquiry;

    if (Buffer == NULL || Length == 0)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return;
    }

    RtlZeroMemory(&Inquiry, sizeof(Inquiry));

    Inquiry.DeviceType = DIRECT_ACCESS_DEVICE;
    Inquiry.DeviceTypeQualifier = DEVICE_CONNECTED;

    /* SPC-4 */
    Inquiry.Versions = 6;
    Inquiry.ResponseDataFormat = 2;
    Inquiry.AdditionalLength = NVME_INQUIRY_DATA_LENGTH -
                               FIELD_OFFSET(INQUIRYDATA, AdditionalLength) - 1;
    Inquiry.CommandQueue = 1;

    RtlFillMemory(Inquiry.VendorId, sizeof(Inquiry.VendorId) +
                  sizeof(Inquiry.ProductId) + sizeof(Inquiry.ProductRevisionLevel), ' ');
    RtlCopyMemory(Inquiry.VendorId, "NVMe", 4);
    RtlCopyMemory(Inquiry.ProductId, Adapter->ModelNumber, sizeof(Inquiry.ProductId));
    NvmpCopyFirmwareRevision(Adapter, Inquiry.ProductRevisionLevel,
                             sizeof(Inquiry.ProductRevisionLevel));

    Length = min(Length, NVME_INQUIRY_DATA_LENGTH);

    RtlCopyMemory(Buffer, &Inquiry, Length);
    SrbSetDataTransferLength(Srb, Length);
    NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
}


/**
 * @brief Writes Length bytes of Id as hex, in groups of two bytes ended by '_' and a final '.'.
 */
static
VOID
NvmpFormatSerial(
    _Out_writes_bytes_((Length / 2) * 5) PUCHAR Target,
    _In_reads_bytes_(Length) const UCHAR *Id,
    _In_ ULONG Length)
{
    ULONG Index;

    for (Index = 0; Index < Length; Index += 2)
    {
        NvmpWriteHex(Target, ((ULONG)Id[Index] << 8) | Id[Index + 1], 4);
        Target[4] = '_';
        Target += 5;
    }

    Target[-1] = '.';
}


/**
 * @brief Returns an INQUIRY VPD page. The data buffer has already been zeroed.
 */
static
VOID
NvmpInquiryVpd(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_NAMESPACE Namespace,
    _In_ UCHAR PageCode,
    _In_ PUCHAR Buffer,
    _In_ ULONG Length)
{
    ULONG Needed;

    switch (PageCode)
    {
        case VPD_SUPPORTED_PAGES:
        {
            static const UCHAR Pages[] = { VPD_SUPPORTED_PAGES,
                                           VPD_SERIAL_NUMBER,
                                           VPD_DEVICE_IDENTIFIERS,
                                           VPD_BLOCK_LIMITS,
                                           VPD_BLOCK_DEVICE_CHARACTERISTICS,
                                           VPD_LOGICAL_BLOCK_PROVISIONING };

            Needed = 4 + sizeof(Pages);
            if (Length < Needed)
                break;

            Buffer[3] = sizeof(Pages);
            RtlCopyMemory(&Buffer[4], Pages, sizeof(Pages));

            SrbSetDataTransferLength(Srb, Needed);
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return;
        }

        case VPD_SERIAL_NUMBER:
        {
            BOOLEAN UseNguid = !NvmpIsZero(Namespace->Nguid, sizeof(Namespace->Nguid));
            ULONG IdLength = UseNguid ? sizeof(Namespace->Nguid) : sizeof(Namespace->Eui64);
            ULONG SerialLength = (IdLength / 2) * 5;

            Needed = 4 + SerialLength;
            if (Length < Needed)
                break;

            Buffer[1] = VPD_SERIAL_NUMBER;
            Buffer[3] = (UCHAR)SerialLength;
            NvmpFormatSerial(&Buffer[4], UseNguid ? Namespace->Nguid : Namespace->Eui64, IdLength);

            SrbSetDataTransferLength(Srb, Needed);
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return;
        }

        case VPD_DEVICE_IDENTIFIERS:
        {
            PVPD_IDENTIFICATION_DESCRIPTOR Descriptor;
            const UCHAR *Eui = NULL;
            ULONG EuiLength = 0;
            ULONG IdentifierLength;
            PUCHAR Identifier;
            ULONG Index;

            /* One SCSI name string from the NGUID, the EUI64, or vendor, model, NSID and serial */
            if (!NvmpIsZero(Namespace->Nguid, sizeof(Namespace->Nguid)))
            {
                Eui = Namespace->Nguid;
                EuiLength = sizeof(Namespace->Nguid);
            }
            else if (!NvmpIsZero(Namespace->Eui64, sizeof(Namespace->Eui64)))
            {
                Eui = Namespace->Eui64;
                EuiLength = sizeof(Namespace->Eui64);
            }

            if (Eui != NULL)
            {
                IdentifierLength = 4 + (2 * EuiLength);
            }
            else
            {
                IdentifierLength = 4 + sizeof(Adapter->ModelNumber) +
                                   4 + sizeof(Adapter->SerialNumber);
            }

            Needed = 8 + IdentifierLength;
            if (Length < Needed)
                break;

            Buffer[1] = VPD_DEVICE_IDENTIFIERS;
            Buffer[3] = (UCHAR)(Needed - 4);

            Descriptor = (PVPD_IDENTIFICATION_DESCRIPTOR)&Buffer[4];
            Descriptor->CodeSet = VpdCodeSetUTF8;
            Descriptor->IdentifierType = VpdIdentifierTypeSCSINameString;
            Descriptor->Association = VpdAssocDevice;
            Descriptor->IdentifierLength = (UCHAR)IdentifierLength;

            Identifier = Descriptor->Identifier;

            if (Eui != NULL)
            {
                RtlCopyMemory(Identifier, "eui.", 4);
                for (Index = 0; Index < EuiLength; Index++)
                    NvmpWriteHex(&Identifier[4 + (2 * Index)], Eui[Index], 2);
            }
            else
            {
                NvmpWriteHex(Identifier, Adapter->VendorId, 4);
                Identifier += 4;
                RtlCopyMemory(Identifier, Adapter->ModelNumber, sizeof(Adapter->ModelNumber));
                Identifier += sizeof(Adapter->ModelNumber);
                NvmpWriteHex(Identifier, (USHORT)Namespace->NamespaceId, 4);
                Identifier += 4;
                RtlCopyMemory(Identifier, Adapter->SerialNumber, sizeof(Adapter->SerialNumber));
            }

            SrbSetDataTransferLength(Srb, Needed);
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return;
        }

        case VPD_BLOCK_LIMITS:
        {
            /* Header only, there is no UNMAP support to describe */
            Needed = 20;
            if (Length < Needed)
                break;

            Buffer[1] = VPD_BLOCK_LIMITS;
            Buffer[3] = (UCHAR)(Needed - 4);

            SrbSetDataTransferLength(Srb, Needed);
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return;
        }

        case VPD_BLOCK_DEVICE_CHARACTERISTICS:
        {
            PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE Characteristics;

            if (Length < 8)
                break;

            Characteristics = (PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE)Buffer;
            Characteristics->PageCode = VPD_BLOCK_DEVICE_CHARACTERISTICS;
            Characteristics->PageLength = 0x3C;

            /* 1 means non rotating medium */
            Characteristics->MediumRotationRateLsb = 1;

            /* The transfer length is left as requested, the rest of the page reads as zero */
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return;
        }

        case VPD_LOGICAL_BLOCK_PROVISIONING:
        {
            PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE Provisioning;

            Needed = 8;
            if (Length < Needed)
                break;

            Provisioning = (PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE)Buffer;
            Provisioning->PageCode = VPD_LOGICAL_BLOCK_PROVISIONING;
            Provisioning->PageLength[1] = 4;

            /* Deallocate is not used, so only thin provisioning is reported */
            if (Namespace->ThinProvisioning)
                Provisioning->ProvisioningType = PROVISIONING_TYPE_THIN;

            SrbSetDataTransferLength(Srb, Needed);
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return;
        }

        default:
            break;
    }

    NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST, SCSI_ADSENSE_INVALID_CDB);
}


/**
 * @brief Handles INQUIRY.
 */
static
VOID
NvmpInquiry(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_NAMESPACE Namespace,
    _In_ PCDB Cdb)
{
    PVOID Buffer = SrbGetDataBuffer(Srb);
    ULONG Length = SrbGetDataTransferLength(Srb);

    if (!Cdb->CDB6INQUIRY3.EnableVitalProductData)
    {
        /* A page code is only valid with EVPD set */
        if (Cdb->CDB6INQUIRY3.PageCode != 0)
        {
            NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                       SCSI_ADSENSE_INVALID_CDB);
            return;
        }

        NvmpInquiryStandard(Adapter, Srb, Buffer, Length);
        return;
    }

    if (Buffer == NULL)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return;
    }

    RtlZeroMemory(Buffer, Length);
    NvmpInquiryVpd(Adapter, Srb, Namespace, Cdb->CDB6INQUIRY3.PageCode, Buffer, Length);
}


/**
 * @brief Handles READ CAPACITY (10) and (16).
 *
 * The (10) form clamps the LBA to 0xFFFFFFFF so the caller moves on to the (16) form.
 */
static
VOID
NvmpReadCapacity(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_NAMESPACE Namespace,
    _In_ PCDB Cdb)
{
    PUCHAR Buffer = SrbGetDataBuffer(Srb);
    ULONG Length = SrbGetDataTransferLength(Srb);
    BOOLEAN Long = (Cdb->CDB6GENERIC.OperationCode == SCSIOP_READ_CAPACITY16);
    ULONGLONG LastBlock = Namespace->BlockCount - 1;

    if (Buffer == NULL || Length < sizeof(READ_CAPACITY_DATA) ||
        (Long && Length < sizeof(READ_CAPACITY_DATA_EX)))
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return;
    }

    if (!Long)
    {
        PREAD_CAPACITY_DATA Capacity = (PREAD_CAPACITY_DATA)Buffer;
        ULONG LastBlock32 = (ULONG)min(LastBlock, MAXULONG);

        REVERSE_BYTES(&Capacity->LogicalBlockAddress, &LastBlock32);
        REVERSE_BYTES(&Capacity->BytesPerBlock, &Namespace->BlockSize);

        SrbSetDataTransferLength(Srb, sizeof(READ_CAPACITY_DATA));
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
        return;
    }

    if (Cdb->READ_CAPACITY16.ServiceAction != SERVICE_ACTION_READ_CAPACITY16)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_ILLEGAL_COMMAND);
        return;
    }

    REVERSE_BYTES_QUAD(&((PREAD_CAPACITY_DATA_EX)Buffer)->LogicalBlockAddress, &LastBlock);
    REVERSE_BYTES(&((PREAD_CAPACITY_DATA_EX)Buffer)->BytesPerBlock, &Namespace->BlockSize);

    /* Protection, exponent and alignment fields, with LBPME set for a thin namespace */
    if (Length >= sizeof(READ_CAPACITY_DATA_EX) + 4)
    {
        Buffer[12] = 0;
        Buffer[13] = 0;
        Buffer[14] = Namespace->ThinProvisioning ? 0x80 : 0;
        Buffer[15] = 0;
        SrbSetDataTransferLength(Srb, sizeof(READ_CAPACITY_DATA_EX) + 4);
    }
    else
    {
        SrbSetDataTransferLength(Srb, sizeof(READ_CAPACITY_DATA_EX));
    }

    NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
}


/**
 * @brief Handles REPORT LUNS, one LUN per namespace the controller reports.
 */
static
VOID
NvmpReportLuns(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb)
{
    PUCHAR Buffer = SrbGetDataBuffer(Srb);
    ULONG Length = SrbGetDataTransferLength(Srb);
    ULONG ListLength = Adapter->NamespaceCount * 8;
    ULONG Lun;

    if (Buffer == NULL || Length < 8 + ListLength)
    {
        /* Only the list length, so the caller can retry with a larger buffer */
        if (Buffer != NULL && Length >= 4)
            REVERSE_BYTES(Buffer, &ListLength);

        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_DATA_OVERRUN);
        return;
    }

    RtlZeroMemory(Buffer, 8 + ListLength);
    REVERSE_BYTES(Buffer, &ListLength);

    /* Single level LUN, peripheral device addressing */
    for (Lun = 0; Lun < Adapter->NamespaceCount; Lun++)
        Buffer[8 + (Lun * 8) + 1] = (UCHAR)Lun;

    NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
}


/**
 * @brief Returns the mode parameter header length for MODE SENSE (6) or (10).
 */
static
ULONG
NvmpModeHeaderLength(
    _In_ PCDB Cdb)
{
    if (Cdb->CDB6GENERIC.OperationCode == SCSIOP_MODE_SENSE)
        return sizeof(MODE_PARAMETER_HEADER);

    return sizeof(MODE_PARAMETER_HEADER10);
}


/**
 * @brief Finishes MODE SENSE once the write cache state is known.
 */
static
VOID
NvmpModeSenseFinish(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb)
{
    ULONG Header = NvmpModeHeaderLength(SrbGetCdb(Srb));
    ULONG Length = SrbGetDataTransferLength(Srb);

    if (Length >= Header + NVME_CACHING_PAGE_LENGTH)
        SrbSetDataTransferLength(Srb, Header + NVME_CACHING_PAGE_LENGTH);
    else
        SrbSetDataTransferLength(Srb, Header + NVME_CACHING_PAGE_MINIMUM);

    NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
}


/**
 * @brief Get Features (Volatile Write Cache) completion for MODE SENSE.
 */
static
VOID
NvmpModeSenseCacheDone(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PNVME_COMPLETION_ENTRY Completion)
{
    PVOID Srb = Context->Srb;
    PMODE_CACHING_PAGE Caching;

    if (Completion->DW3.Status.SC != NVME_STATUS_SUCCESS_COMPLETION ||
        Completion->DW3.Status.SCT != NVME_STATUS_TYPE_GENERIC_COMMAND)
    {
        NvmpCompleteWithStatus(Adapter, Srb, Completion->DW3.Status);
        return;
    }

    Caching = (PMODE_CACHING_PAGE)((PUCHAR)SrbGetDataBuffer(Srb) +
                                   NvmpModeHeaderLength(SrbGetCdb(Srb)));
    Caching->WriteCacheEnable = (Completion->DW0 & 1);

    NvmpModeSenseFinish(Adapter, Srb);
}


/**
 * @brief SMART / Health Information completion for MODE SENSE, builds the caching page.
 */
static
VOID
NvmpModeSenseHealthDone(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PNVME_COMPLETION_ENTRY Completion)
{
    PVOID Srb = Context->Srb;
    PUCHAR Buffer = SrbGetDataBuffer(Srb);
    ULONG Length = SrbGetDataTransferLength(Srb);
    ULONG Header = NvmpModeHeaderLength(SrbGetCdb(Srb));
    PNVME_HEALTH_INFO_LOG Health = NvmpContextPage(Context);
    PMODE_CACHING_PAGE Caching;
    UCHAR DeviceSpecific;
    ULONG PageLength;

    if (Completion->DW3.Status.SC != NVME_STATUS_SUCCESS_COMPLETION ||
        Completion->DW3.Status.SCT != NVME_STATUS_TYPE_GENERIC_COMMAND)
    {
        NvmpCompleteWithStatus(Adapter, Srb, Completion->DW3.Status);
        return;
    }

    RtlZeroMemory(Buffer, Length);

    DeviceSpecific = MODE_DSP_FUA_SUPPORTED;
    if (Health->CriticalWarning.ReadOnly)
        DeviceSpecific |= MODE_DSP_WRITE_PROTECT;

    if (Length >= Header + NVME_CACHING_PAGE_LENGTH)
        PageLength = NVME_CACHING_PAGE_LENGTH;
    else
        PageLength = NVME_CACHING_PAGE_MINIMUM;

    if (Header == sizeof(MODE_PARAMETER_HEADER))
    {
        PMODE_PARAMETER_HEADER Header6 = (PMODE_PARAMETER_HEADER)Buffer;

        Header6->ModeDataLength = (UCHAR)(sizeof(*Header6) - 1 + PageLength);
        Header6->DeviceSpecificParameter = DeviceSpecific;
    }
    else
    {
        PMODE_PARAMETER_HEADER10 Header10 = (PMODE_PARAMETER_HEADER10)Buffer;
        USHORT DataLength;

        DataLength = (USHORT)(sizeof(*Header10) - sizeof(Header10->ModeDataLength) + PageLength);
        REVERSE_BYTES_SHORT(Header10->ModeDataLength, &DataLength);
        Header10->DeviceSpecificParameter = DeviceSpecific;
    }

    /* The page length always describes the short form */
    Caching = (PMODE_CACHING_PAGE)(Buffer + Header);
    Caching->PageCode = MODE_PAGE_CACHING;
    Caching->PageLength = NVME_CACHING_PAGE_MINIMUM - 2;

    if (!Adapter->VolatileWriteCache)
    {
        NvmpModeSenseFinish(Adapter, Srb);
        return;
    }

    /* Whether the cache is enabled, not just present */
    RtlZeroMemory(&Context->Command, sizeof(Context->Command));
    Context->Command.CDW0.OPC = NVME_ADMIN_COMMAND_GET_FEATURES;
    Context->Command.u.GETFEATURES.CDW10.FID = NVME_FEATURE_VOLATILE_WRITE_CACHE;
    Context->AdminCompletion = NvmpModeSenseCacheDone;

    if (!NvmpSubmitAdminRequest(Adapter, Context))
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_ERROR);
}


/**
 * @brief Handles MODE SENSE (6) and (10), only current values of the caching page.
 *
 * @return TRUE if an admin command was built to read the drive state.
 */
static
BOOLEAN
NvmpBuildModeSense(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PCDB Cdb)
{
    PVOID Buffer = SrbGetDataBuffer(Srb);
    ULONG Length = SrbGetDataTransferLength(Srb);
    ULONG Header = NvmpModeHeaderLength(Cdb);
    PHYSICAL_ADDRESS Address;
    ULONG PhysicalLength;
    UCHAR PageCode;

    /* Both CDB forms keep the page control and page code in byte 2 */
    if (Cdb->MODE_SENSE.Pc != MODE_SENSE_CURRENT_VALUES)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return FALSE;
    }

    PageCode = Cdb->MODE_SENSE.PageCode;

    if (Buffer == NULL || Length < Header ||
        (PageCode != MODE_PAGE_CACHING && PageCode != MODE_SENSE_RETURN_ALL))
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return FALSE;
    }

    if (Length - Header < NVME_CACHING_PAGE_MINIMUM)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_DATA_OVERRUN,
                                   SCSI_ADSENSE_INVALID_CDB);
        return FALSE;
    }

    /* The read only critical warning becomes the write protect bit */
    Address = StorPortGetPhysicalAddress(Adapter, NULL, NvmpContextPage(Context), &PhysicalLength);

    Context->Command.CDW0.OPC = NVME_ADMIN_COMMAND_GET_LOG_PAGE;
    Context->Command.NSID = NVME_NAMESPACE_ALL;
    Context->Command.PRP1 = Address.QuadPart;
    Context->Command.u.GETLOGPAGE.CDW10.LID = NVME_LOG_PAGE_HEALTH_INFO;
    Context->Command.u.GETLOGPAGE.CDW10.NUMD = (NVME_HEALTH_LOG_LENGTH / sizeof(ULONG)) - 1;
    Context->AdminCompletion = NvmpModeSenseHealthDone;

    return TRUE;
}


/**
 * @brief Set Features (Power Management) completion for START STOP UNIT.
 */
static
VOID
NvmpStartStopDone(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PNVME_COMPLETION_ENTRY Completion)
{
    if (Completion->DW3.Status.SC != NVME_STATUS_SUCCESS_COMPLETION ||
        Completion->DW3.Status.SCT != NVME_STATUS_TYPE_GENERIC_COMMAND)
    {
        NvmpCompleteWithStatus(Adapter, Context->Srb, Completion->DW3.Status);
        return;
    }

    NvmpCompleteRequest(Adapter, Context->Srb, SRB_STATUS_SUCCESS);
}


/**
 * @brief Handles START STOP UNIT by moving the controller to power state 0 or the lowest one.
 *
 * Nothing is done when the controller has more than one namespace, since power is shared.
 *
 * @return TRUE if an admin command was built.
 */
static
BOOLEAN
NvmpBuildStartStop(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PCDB Cdb)
{
    if (Cdb->START_STOP.Immediate)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return FALSE;
    }

    if (Adapter->NamespaceCount > 1)
    {
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
        return FALSE;
    }

    /* NPSS is 0 based, so it names the lowest power state */
    Context->Command.CDW0.OPC = NVME_ADMIN_COMMAND_SET_FEATURES;
    Context->Command.u.SETFEATURES.CDW10.FID = NVME_FEATURE_POWER_MANAGEMENT;
    Context->Command.u.SETFEATURES.CDW11.PowerManagement.PS =
        Cdb->START_STOP.Start ? 0 : Adapter->PowerStateCount;
    Context->AdminCompletion = NvmpStartStopDone;

    return TRUE;
}


/**
 * @brief Translates SRB_FUNCTION_EXECUTE_SCSI.
 *
 * @return TRUE if a command was built for StartIo to post, FALSE if the request was completed.
 */
BOOLEAN
NvmpBuildScsi(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb)
{
    PNVME_REQUEST_CONTEXT Context = SrbGetMiniportContext(Srb);
    PNVME_NAMESPACE Namespace;
    PCDB Cdb;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    RtlZeroMemory(Context, sizeof(*Context));
    Context->Srb = Srb;

    Cdb = SrbGetCdb(Srb);
    SrbGetPathTargetLun(Srb, &PathId, &TargetId, &Lun);
    Namespace = NvmpNamespaceFromLun(Adapter, Lun);

    if (Adapter->State != NvmeAdapterRunning || PathId != 0 || TargetId != 0 ||
        Namespace == NULL || Cdb == NULL || SrbGetCdbLength(Srb) < 6)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_NO_DEVICE,
                                   SCSI_ADSENSE_ILLEGAL_COMMAND);
        return FALSE;
    }

    switch (Cdb->CDB6GENERIC.OperationCode)
    {
        case SCSIOP_TEST_UNIT_READY:
        case SCSIOP_VERIFY:
        case SCSIOP_VERIFY16:
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return FALSE;

        case SCSIOP_INQUIRY:
            NvmpInquiry(Adapter, Srb, Namespace, Cdb);
            return FALSE;

        case SCSIOP_READ_CAPACITY:
        case SCSIOP_READ_CAPACITY16:
            NvmpReadCapacity(Adapter, Srb, Namespace, Cdb);
            return FALSE;

        case SCSIOP_REPORT_LUNS:
            NvmpReportLuns(Adapter, Srb);
            return FALSE;

        case SCSIOP_MODE_SENSE:
        case SCSIOP_MODE_SENSE10:
            return NvmpBuildModeSense(Adapter, Srb, Context, Cdb);

        case SCSIOP_START_STOP_UNIT:
            return NvmpBuildStartStop(Adapter, Srb, Context, Cdb);

        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            return NvmpBuildReadWrite(Adapter, Srb, Namespace, Context, Cdb);

        case SCSIOP_SYNCHRONIZE_CACHE:
            return NvmpBuildFlush(Adapter, Srb);

        default:
            NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                       SCSI_ADSENSE_ILLEGAL_COMMAND);
            return FALSE;
    }
}

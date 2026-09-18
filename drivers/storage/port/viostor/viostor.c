/*
 * Reduced single-queue legacy VirtIO-blk Storport miniport.
 *
 * The VirtIO request wire format follows the BSD-3-Clause virtio-win viostor
 * source at commit a66c7af4aef5ded62b2b19048df7e9c396b927ef. The Storport
 * integration is ReactOS-specific and intentionally supports only the legacy
 * PCI interface (1AF4:1001), one split queue, and line interrupts.
 *
 * The virtqueue request/completion model (opaque request pointer returned by
 * virtqueue_get_buf, descriptor-chain ownership, used-ring draining, and
 * event-index interrupt suppression rules) references the FreeBSD
 * virtio_blk driver at:
 *   https://cgit.freebsd.org/src/plain/sys/dev/virtio/block/virtio_blk.c
 * The FreeBSD bus/DMA/interrupt plumbing is not directly portable to
 * ReactOS StorPort and is used only as a protocol-level reference.
 */
#include "viostor.h"
#include <kdebugprint.h>
#include <ntstrsafe.h>


static u8 VioReadByte(ULONG_PTR Register)
{
    return READ_PORT_UCHAR((PUCHAR)Register);
}

static u16 VioReadWord(ULONG_PTR Register)
{
    return READ_PORT_USHORT((PUSHORT)Register);
}

static u32 VioReadDword(ULONG_PTR Register)
{
    return READ_PORT_ULONG((PULONG)Register);
}

static void VioWriteByte(ULONG_PTR Register, u8 Value)
{
    WRITE_PORT_UCHAR((PUCHAR)Register, Value);
}

static void VioWriteWord(ULONG_PTR Register, u16 Value)
{
    WRITE_PORT_USHORT((PUSHORT)Register, Value);
}

static void VioWriteDword(ULONG_PTR Register, u32 Value)
{
    WRITE_PORT_ULONG((PULONG)Register, Value);
}
static void *VioAllocateContiguous(void *Context, size_t Size)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    if (Adapter->QueueMemoryUsed || Size > VIOSTOR_QUEUE_MEMORY_SIZE ||
        Adapter->QueueMemory == NULL)
        return NULL;
    Adapter->QueueMemoryUsed = TRUE;
    RtlZeroMemory(Adapter->QueueMemory, Size);
    return Adapter->QueueMemory;
}

static void VioFreeContiguous(void *Context, void *Virtual)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    if (Virtual == Adapter->QueueMemory)
        Adapter->QueueMemoryUsed = FALSE;
}

static ULONGLONG VioGetPhysical(void *Context, void *Virtual)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;
    ULONG Length;
    PHYSICAL_ADDRESS Address;

    Length = 0;
    Address = StorPortGetPhysicalAddress(Adapter, NULL, Virtual, &Length);
    return Address.QuadPart;
}

static void *VioAllocatePool(void *Context, size_t Size)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;
    PVOID Buffer;

    Buffer = NULL;
    if (StorPortAllocatePool(Adapter, (ULONG)Size, VIOSTOR_POOL_TAG, &Buffer) != 0)
        return NULL;
    if (Buffer != NULL)
        RtlZeroMemory(Buffer, Size);
    return Buffer;
}

static void VioFreePool(void *Context, void *Address)
{
    if (Address != NULL)
        StorPortFreePool(Context, Address);
}

/* The VirtIO library only needs PCI capability reads during modern probing.
 * We reject modern capabilities below and use this cached PCI header for the
 * required callback, avoiding a non-existent StorPortReadPciSlotInformation API.
 */
static int VioReadPciByte(void *Context, int Where, u8 *Value)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    if (Where < 0 || (ULONG)Where + sizeof(*Value) > sizeof(Adapter->PciConfig))
        return -1;
    *Value = *((PUCHAR)&Adapter->PciConfig + Where);
    return 0;
}

static int VioReadPciWord(void *Context, int Where, u16 *Value)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    if (Where < 0 || (ULONG)Where + sizeof(*Value) > sizeof(Adapter->PciConfig))
        return -1;
    RtlCopyMemory(Value, (PUCHAR)&Adapter->PciConfig + Where, sizeof(*Value));
    return 0;
}

static int VioReadPciDword(void *Context, int Where, u32 *Value)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    if (Where < 0 || (ULONG)Where + sizeof(*Value) > sizeof(Adapter->PciConfig))
        return -1;
    RtlCopyMemory(Value, (PUCHAR)&Adapter->PciConfig + Where, sizeof(*Value));
    return 0;
}

static size_t VioGetResourceLength(void *Context, int Bar)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    return Bar == 0 ? Adapter->IoLength : 0;
}

static void *VioMapAddressRange(void *Context, int Bar, size_t Offset, size_t MaxLength)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = Context;

    if (Bar != 0 || Adapter->IoBase == NULL ||
        Offset > Adapter->IoLength ||
        MaxLength > Adapter->IoLength - Offset)
        return NULL;
    return (PUCHAR)Adapter->IoBase + Offset;
}

static u16 VioGetMsixVector(void *Context, int Queue)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Queue);
    return VIRTIO_MSI_NO_VECTOR;
}

static void VioSleep(void *Context, unsigned int Milliseconds)
{
    UNREFERENCED_PARAMETER(Context);
    StorPortStallExecution(Milliseconds * 1000);
}

static const VirtIOSystemOps VioSystemOps = {
    VioReadByte, VioReadWord, VioReadDword,
    VioWriteByte, VioWriteWord, VioWriteDword,
    VioAllocateContiguous, VioFreeContiguous, VioGetPhysical,
    VioAllocatePool, VioFreePool,
    VioReadPciByte, VioReadPciWord, VioReadPciDword,
    VioGetResourceLength, VioMapAddressRange,
    VioGetMsixVector, VioSleep
};

static ULONG VioLoadBe32(const UCHAR *Bytes)
{
    return ((ULONG)Bytes[0] << 24) | ((ULONG)Bytes[1] << 16) |
           ((ULONG)Bytes[2] << 8) | Bytes[3];
}

static ULONGLONG VioLoadBe64(const UCHAR *Bytes)
{
    ULONGLONG Value;
    ULONG Index;

    Value = 0;
    for (Index = 0; Index < 8; Index++)
        Value = (Value << 8) | Bytes[Index];
    return Value;
}
static void VioStoreBe16(UCHAR *Bytes, USHORT Value)
{
    Bytes[0] = (UCHAR)(Value >> 8);
    Bytes[1] = (UCHAR)Value;
}


static void VioStoreBe32(UCHAR *Bytes, ULONG Value)
{
    Bytes[0] = (UCHAR)(Value >> 24);
    Bytes[1] = (UCHAR)(Value >> 16);
    Bytes[2] = (UCHAR)(Value >> 8);
    Bytes[3] = (UCHAR)Value;
}

static void VioStoreBe64(UCHAR *Bytes, ULONGLONG Value)
{
    LONG Index;

    for (Index = 7; Index >= 0; Index--) {
        Bytes[Index] = (UCHAR)Value;
        Value >>= 8;
    }
}
static BOOLEAN VioIsFlushCommand(PSCSI_REQUEST_BLOCK Srb)
{
    return Srb->Function == SRB_FUNCTION_FLUSH ||
           Srb->Function == SRB_FUNCTION_SHUTDOWN ||
           Srb->Cdb[0] == SCSIOP_SYNCHRONIZE_CACHE ||
           Srb->Cdb[0] == SCSIOP_SYNCHRONIZE_CACHE16;
}

/*
 * Each SRB carries its own VIOSTOR_SRB_EXTENSION in Srb->SrbExtension
 * (allocated by storport's PortFdoScsi alongside the STOR_REQUEST_CONTEXT).
 * The miniport no longer serializes through a single Adapter->Request slot;
 * multiple requests may be in flight on the virtqueue simultaneously.
 */
static PVIOSTOR_SRB_EXTENSION VioGetRequest(
    PVIOSTOR_ADAPTER_EXTENSION Adapter,
    PSCSI_REQUEST_BLOCK Srb)
{
    UNREFERENCED_PARAMETER(Adapter);
    return (PVIOSTOR_SRB_EXTENSION)Srb->SrbExtension;
}

static void VioReleaseRequest(PVIOSTOR_ADAPTER_EXTENSION Adapter,
                              PVIOSTOR_SRB_EXTENSION Request)
{
    UNREFERENCED_PARAMETER(Adapter);
    if (Request->ScatterGather != NULL) {
        StorPortPutScatterGatherList(
            Adapter,
            Request->ScatterGather,
            Request->Header.Type == VIRTIO_BLK_T_OUT);
        Request->ScatterGather = NULL;
    }
    Request->Srb = NULL;
    Request->SgCount = 0;
    Request->TransferLength = 0;
}


static void VioComplete(PVIOSTOR_ADAPTER_EXTENSION Adapter,
                        PSCSI_REQUEST_BLOCK Srb, UCHAR Status)
{
    DPrintf(1, "viostor: complete function=0x%02x opcode=0x%02x status=0x%02x scsi=0x%02x transfer=%lu\n",
            Srb->Function, Srb->Cdb[0], Status,
            Status == SRB_STATUS_SUCCESS ? SCSISTAT_GOOD : Srb->ScsiStatus,
            Srb->DataTransferLength);
    Srb->SrbStatus = Status;
    if (Status == SRB_STATUS_SUCCESS)
        Srb->ScsiStatus = SCSISTAT_GOOD;
    StorPortNotification(RequestComplete, Adapter, Srb);
}


static BOOLEAN VioLocalCommand(PVIOSTOR_ADAPTER_EXTENSION Adapter,
                               PSCSI_REQUEST_BLOCK Srb)
{
    PUCHAR Buffer;
    ULONG Length;
    UCHAR OpCode;
    ULONG Blocks;
    ULONGLONG Lba;
    ULONGLONG TransferLength;
    READ_CAPACITY_DATA Capacity;
    READ_CAPACITY_DATA_EX CapacityEx;
    INQUIRYDATA Inquiry;

    Buffer = (PUCHAR)Srb->DataBuffer;
    Length = Srb->DataTransferLength;
    OpCode = Srb->Cdb[0];
    if (virtio_is_feature_enabled(Adapter->Features, VIRTIO_BLK_F_RO) &&
        (OpCode == SCSIOP_WRITE || OpCode == SCSIOP_WRITE12 ||
         OpCode == SCSIOP_WRITE16 || OpCode == SCSIOP_WRITE_VERIFY ||
         OpCode == SCSIOP_WRITE_VERIFY12 || OpCode == SCSIOP_WRITE_VERIFY16)) {
        DPrintf(0, "viostor: reject write on read-only device opcode=0x%02x\n",
                OpCode);
        VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
        return TRUE;
    }
    DPrintf(1, "viostor: local opcode=0x%02x cdb_length=%u data_length=%lu\n",
            OpCode, Srb->CdbLength, Length);
    switch (OpCode) {
    case SCSIOP_VERIFY:
    case SCSIOP_VERIFY12:
    case SCSIOP_VERIFY16:
        /*
         * FAT setup can issue a media verify after its first metadata write.
         * A no-data VERIFY is a valid success path for a block device; a
         * BYTCHK/data-bearing verify needs a real read/compare implementation.
         */
        if (Length != 0 || (Srb->Cdb[1] & 0x02) != 0) {
            DPrintf(0, "viostor: verify unsupported opcode=0x%02x flags=0x%08lx length=%lu\n",
                    OpCode, Srb->SrbFlags, Length);
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;
    case SCSIOP_TEST_UNIT_READY:
    case SCSIOP_START_STOP_UNIT:
        if (Length != 0) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        Srb->DataTransferLength = 0;
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;

    case SCSIOP_SYNCHRONIZE_CACHE:
    case SCSIOP_SYNCHRONIZE_CACHE16:
        if (!virtio_is_feature_enabled(Adapter->Features,
                                       VIRTIO_BLK_F_FLUSH)) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        /*
         * SYNCHRONIZE CACHE has no data phase. Some class-driver paths
         * leave a stale transfer length in the SRB; discard it before
         * submitting the virtio-blk FLUSH request.
         */
        Srb->DataTransferLength = 0;
        Lba = 0;
        Blocks = 0;
        break;

    case SCSIOP_INQUIRY:
    {
        UCHAR VpdPage;
        UCHAR AllocationLength;
        UCHAR Vpd[64];
        ULONG ResponseLength;
        ULONG SerialLength;

        if (Buffer == NULL || Srb->CdbLength < 6 ||
            (Srb->Cdb[1] & (UCHAR)~1u) != 0) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        AllocationLength = Srb->Cdb[4];
        if ((Srb->Cdb[1] & 1) == 0) {
            if (Srb->Cdb[2] != 0) {
                VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
                return TRUE;
            }
            RtlZeroMemory(&Inquiry, sizeof(Inquiry));
            Inquiry.DeviceType = DIRECT_ACCESS_DEVICE;
            Inquiry.Versions = 2;
            Inquiry.ResponseDataFormat = 2;
            Inquiry.AdditionalLength = sizeof(INQUIRYDATA) - 5;
            RtlCopyMemory(Inquiry.VendorId, "ReactOS ", sizeof(Inquiry.VendorId));
            RtlCopyMemory(Inquiry.ProductId, "VirtIO Block     ", sizeof(Inquiry.ProductId));
            RtlCopyMemory(Inquiry.ProductRevisionLevel, "0001", sizeof(Inquiry.ProductRevisionLevel));
            ResponseLength = min((ULONG)sizeof(Inquiry), (ULONG)AllocationLength);
            ResponseLength = min(ResponseLength, Length);
            if (ResponseLength != 0)
                RtlCopyMemory(Buffer, &Inquiry, ResponseLength);
            Srb->DataTransferLength = ResponseLength;
            VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
            return TRUE;
        }

        VpdPage = Srb->Cdb[2];
        RtlZeroMemory(Vpd, sizeof(Vpd));
        Vpd[0] = DIRECT_ACCESS_DEVICE;
        Vpd[1] = VpdPage;
        switch (VpdPage) {
        case VPD_SUPPORTED_PAGES:
            Vpd[4] = 3;
            Vpd[5] = VPD_SUPPORTED_PAGES;
            Vpd[6] = VPD_SERIAL_NUMBER;
            Vpd[7] = VPD_DEVICE_IDENTIFIERS;
            ResponseLength = 8;
            break;
        case VPD_SERIAL_NUMBER:
            SerialLength = (ULONG)strlen((PCSTR)Adapter->Serial);
            Vpd[4] = (UCHAR)SerialLength;
            RtlCopyMemory(&Vpd[5], Adapter->Serial, SerialLength);
            ResponseLength = 5 + SerialLength;
            break;
        case VPD_DEVICE_IDENTIFIERS:
            SerialLength = (ULONG)strlen((PCSTR)Adapter->Serial);
            Vpd[4] = (UCHAR)(4 + SerialLength);
            Vpd[5] = VpdCodeSetAscii |
                     (VpdIdentifierTypeVendorSpecific << 4);
            Vpd[6] = 0;
            Vpd[7] = (UCHAR)SerialLength;
            RtlCopyMemory(&Vpd[8], Adapter->Serial, SerialLength);
            ResponseLength = 8 + SerialLength;
            break;
        default:
            DPrintf(0, "viostor: inquiry unsupported vpd=0x%02x allocation=%u\n",
                    VpdPage, AllocationLength);
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        ResponseLength = min(ResponseLength, (ULONG)AllocationLength);
        ResponseLength = min(ResponseLength, Length);
        if (ResponseLength != 0)
            RtlCopyMemory(Buffer, Vpd, ResponseLength);
        Srb->DataTransferLength = ResponseLength;
        DPrintf(0, "viostor: inquiry evpd=0x%02x allocation=%u response=%lu\n",
                VpdPage, AllocationLength, ResponseLength);
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;
    }

    case SCSIOP_REQUEST_SENSE:
        if (Buffer == NULL || Length == 0) {
            VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
            return TRUE;
        }
        RtlZeroMemory(Buffer, Length);
        if (Length >= 14) {
            Buffer[0] = 0x70;
            Buffer[7] = 10;
        }
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;

    case SCSIOP_READ_CAPACITY:
        if (Buffer == NULL || Length < sizeof(Capacity)) {
            VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
            return TRUE;
        }
        VioStoreBe32((PUCHAR)&Capacity.LogicalBlockAddress,
                     Adapter->LastLba > 0xffffffffULL ? 0xffffffffUL : (ULONG)Adapter->LastLba);
        VioStoreBe32((PUCHAR)&Capacity.BytesPerBlock, VIOSTOR_SECTOR_SIZE);
        RtlCopyMemory(Buffer, &Capacity, sizeof(Capacity));
        Srb->DataTransferLength = sizeof(Capacity);
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;

    case SCSIOP_READ_CAPACITY16:
        if (Buffer == NULL || Length < sizeof(CapacityEx) || Srb->Cdb[1] != 0x10) {
            VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
            return TRUE;
        }
        VioStoreBe64((PUCHAR)&CapacityEx.LogicalBlockAddress, Adapter->LastLba);
        VioStoreBe32((PUCHAR)&CapacityEx.BytesPerBlock, VIOSTOR_SECTOR_SIZE);
        RtlCopyMemory(Buffer, &CapacityEx, sizeof(CapacityEx));
        Srb->DataTransferLength = sizeof(CapacityEx);
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;

    case SCSIOP_MODE_SENSE:
    case SCSIOP_MODE_SENSE10:
    {
        ULONG HeaderLength;
        ULONG PageOffset;
        ULONG TotalLength;
        PUCHAR Page;

        if (Buffer == NULL || Length == 0) {
            VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
            return TRUE;
        }
        HeaderLength = OpCode == SCSIOP_MODE_SENSE10 ? 8 : 4;
        PageOffset = HeaderLength;
        RtlZeroMemory(Buffer, Length);
        /* Device-Specific Parameter byte: WP bit (bit 7) for a read-only
           device. Byte 2 in the 6-byte header, byte 3 in the 10-byte one. */
        if (virtio_is_feature_enabled(Adapter->Features, VIRTIO_BLK_F_RO)) {
            if (OpCode == SCSIOP_MODE_SENSE10) {
                if (Length > 3)
                    Buffer[3] |= 0x80;
            } else {
                if (Length > 2)
                    Buffer[2] |= 0x80;
            }
        }
        if ((Srb->Cdb[2] & 0x3f) == 0x08 ||
            (Srb->Cdb[2] & 0x3f) == 0x3f) {
            TotalLength = HeaderLength + 2 + 18;
            if (Length < TotalLength) {
                VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
                return TRUE;
            }
            Page = Buffer + PageOffset;
            Page[0] = 0x08;
            Page[1] = 18;
            if (virtio_is_feature_enabled(Adapter->Features,
                                          VIRTIO_BLK_F_CONFIG_WCE))
                Page[2] = 0x04;
            if (OpCode == SCSIOP_MODE_SENSE10)
                VioStoreBe16(Buffer, (USHORT)(TotalLength - 2));
            else
                Buffer[0] = (UCHAR)(TotalLength - 1);
            Srb->DataTransferLength = TotalLength;
        } else {
            Srb->DataTransferLength = HeaderLength;
        }
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;
    }

    case SCSIOP_READ:
    case SCSIOP_WRITE:
    case SCSIOP_WRITE_VERIFY:
        if (Srb->CdbLength < 10) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        Lba = VioLoadBe32(&Srb->Cdb[2]);
        Blocks = ((ULONG)Srb->Cdb[7] << 8) | Srb->Cdb[8];
        break;

    case SCSIOP_READ12:
    case SCSIOP_WRITE12:
    case SCSIOP_WRITE_VERIFY12:
        if (Srb->CdbLength < 12) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        Lba = VioLoadBe32(&Srb->Cdb[2]);
        Blocks = ((ULONG)Srb->Cdb[6] << 24) | ((ULONG)Srb->Cdb[7] << 16) |
                 ((ULONG)Srb->Cdb[8] << 8) | Srb->Cdb[9];
        break;

    case SCSIOP_READ16:
    case SCSIOP_WRITE16:
    case SCSIOP_WRITE_VERIFY16:
        if (Srb->CdbLength < 16) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        Lba = VioLoadBe64(&Srb->Cdb[2]);
        Blocks = ((ULONG)Srb->Cdb[10] << 24) | ((ULONG)Srb->Cdb[11] << 16) |
                 ((ULONG)Srb->Cdb[12] << 8) | Srb->Cdb[13];
        break;

    default:
        VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
        return TRUE;
    }
    if (VioIsFlushCommand(Srb))
        return FALSE;
    if (Blocks == 0 || Lba >= Adapter->Config.Capacity ||
        Blocks > Adapter->Config.Capacity - Lba ||
        Blocks > MAXULONG / VIOSTOR_SECTOR_SIZE) {
        DPrintf(0, "viostor: reject range opcode=0x%02x lba=%I64u blocks=%u capacity=%I64u\n",
                OpCode, Lba, Blocks, Adapter->Config.Capacity);
        VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
        return TRUE;
    }
    TransferLength = (ULONGLONG)Blocks * VIOSTOR_SECTOR_SIZE;
    /* DataBuffer is only a virtual-address hint: with MapBuffers the port
       may leave it NULL and the DMA path is the SG list fetched in
       VioSubmitIo (StorPortGetScatterGatherList). Do not reject data
       transfers merely because it is NULL. */
    if (TransferLength != Length ||
        TransferLength > Adapter->MaximumTransferLength) {
        DPrintf(0, "viostor: reject transfer opcode=0x%02x transferlen=%I64u length=%lu maxtransfer=%lu buffer=%p\n",
                OpCode, TransferLength, Length, Adapter->MaximumTransferLength, Buffer);
        VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
        return TRUE;
    }
    return FALSE;
}

static BOOLEAN VioSubmitIo(PVIOSTOR_ADAPTER_EXTENSION Adapter,
                           PSCSI_REQUEST_BLOCK Srb)
{
    PVIOSTOR_SRB_EXTENSION Request;
    PSTOR_SCATTER_GATHER_LIST ScatterGather;
    PHYSICAL_ADDRESS Physical;
    ULONG Length;
    ULONG Index;
    ULONG DataCount;
    ULONG DataBytes;
    ULONG OutCount;
    ULONG InCount;
    UCHAR Type;
    ULONGLONG Sector;
    BOOLEAN DataOut;
    BOOLEAN DataIn;
    BOOLEAN QueueAdded;

    Request = VioGetRequest(Adapter, Srb);
    RtlZeroMemory(Request, sizeof(*Request));
    Request->Srb = Srb;
    Request->Status = VIRTIO_BLK_S_IOERR;
    Request->TransferLength = Srb->DataTransferLength;
    DataOut = (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) != 0;
    DataIn = (Srb->SrbFlags & SRB_FLAGS_DATA_IN) != 0;
    if (Srb->DataTransferLength != 0 && DataOut && DataIn) {
        DPrintf(0, "viostor: submit invalid direction opcode=0x%02x flags=0x%08lx in=%u out=%u\n",
                Srb->Cdb[0], Srb->SrbFlags, DataIn, DataOut);
        VioReleaseRequest(Adapter, Request);
        return FALSE;
    }

    if (VioIsFlushCommand(Srb)) {
        Type = VIRTIO_BLK_T_FLUSH;
        Sector = 0;
    } else {
        if (Srb->DataTransferLength != 0 && DataOut)
            Type = VIRTIO_BLK_T_OUT;
        else if (Srb->DataTransferLength != 0 && DataIn)
            Type = VIRTIO_BLK_T_IN;
        else
            Type = (Srb->Cdb[0] == SCSIOP_WRITE ||
                    Srb->Cdb[0] == SCSIOP_WRITE12 ||
                    Srb->Cdb[0] == SCSIOP_WRITE16 ||
                    Srb->Cdb[0] == SCSIOP_WRITE_VERIFY ||
                    Srb->Cdb[0] == SCSIOP_WRITE_VERIFY12 ||
                    Srb->Cdb[0] == SCSIOP_WRITE_VERIFY16) ?
                   VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
        Sector = (Srb->Cdb[0] == SCSIOP_WRITE16 ||
                  Srb->Cdb[0] == SCSIOP_READ16 ||
                  Srb->Cdb[0] == SCSIOP_WRITE_VERIFY16) ?
                 VioLoadBe64(&Srb->Cdb[2]) : VioLoadBe32(&Srb->Cdb[2]);
    }
    DPrintf(1, "viostor: submit opcode=0x%02x flags=0x%08lx data_in=%u data_out=%u type=%u sector=%I64u bytes=%lu\n",
            Srb->Cdb[0], Srb->SrbFlags, DataIn, DataOut, Type, Sector,
            Srb->DataTransferLength);
    Request->Header.Type = Type;
    Request->Header.IoPriority = 0;
    Request->Header.Sector = Sector;

    Physical = StorPortGetPhysicalAddress(Adapter, NULL, &Request->Header, &Length);
    if (Physical.QuadPart == 0 || Length < sizeof(Request->Header)) {
        DPrintf(0, "viostor: submit failed header-physical=%I64x length=%lu\n",
                Physical.QuadPart, Length);
        VioReleaseRequest(Adapter, Request);
        return FALSE;
    }
    Request->Sg[0].physAddr = Physical;
    Request->Sg[0].length = sizeof(Request->Header);
    Request->SgCount = 1;
    OutCount = 1;
    InCount = 0;
    ScatterGather = NULL;
    if (Srb->DataTransferLength != 0) {
        ScatterGather = StorPortGetScatterGatherList(Adapter, Srb);
        Request->ScatterGather = ScatterGather;
        if (ScatterGather == NULL || ScatterGather->NumberOfElements == 0 ||
            ScatterGather->NumberOfElements > Adapter->NumberOfPhysicalBreaks ||
            ScatterGather->NumberOfElements + 2 > VIOSTOR_MAX_SG) {
            DPrintf(0, "viostor: submit failed scatter-list=%p count=%lu max=%lu sg-capacity=%u\n",
                    ScatterGather,
                    ScatterGather != NULL ? ScatterGather->NumberOfElements : 0,
                    Adapter->NumberOfPhysicalBreaks, VIOSTOR_MAX_SG - 2);
            VioReleaseRequest(Adapter, Request);
            return FALSE;
        }
        DataCount = ScatterGather->NumberOfElements;
        DataBytes = 0;
        for (Index = 0; Index < DataCount; Index++) {
            if (ScatterGather->List[Index].PhysicalAddress.QuadPart == 0 ||
                ScatterGather->List[Index].Length == 0 ||
                ScatterGather->List[Index].Length > Srb->DataTransferLength ||
                DataBytes > Srb->DataTransferLength -
                             ScatterGather->List[Index].Length) {
                DPrintf(0, "viostor: submit failed scatter[%lu] phys=%I64x length=%lu bytes=%lu\n",
                        Index, ScatterGather->List[Index].PhysicalAddress.QuadPart,
                        ScatterGather->List[Index].Length, DataBytes);
                VioReleaseRequest(Adapter, Request);
                return FALSE;
            }
            Request->Sg[Request->SgCount].physAddr =
                ScatterGather->List[Index].PhysicalAddress;
            Request->Sg[Request->SgCount].length =
                ScatterGather->List[Index].Length;
            Request->SgCount++;
            DataBytes += ScatterGather->List[Index].Length;
        }
        if (DataBytes != Srb->DataTransferLength) {
            DPrintf(0, "viostor: submit failed scatter-bytes=%lu expected=%lu\n",
                    DataBytes, Srb->DataTransferLength);
            VioReleaseRequest(Adapter, Request);
            return FALSE;
        }
        if (Type == VIRTIO_BLK_T_OUT)
            OutCount += DataCount;
        else
            InCount += DataCount;
        /*
         * The SG list was only needed to fill Request->Sg.  Free it now
         * at DISPATCH_LEVEL (HwStartIo runs at DISPATCH) rather than
         * deferring to the ISR, which runs at DIRQL where
         * ExFreePoolWithTag is not allowed.
         */
        StorPortPutScatterGatherList(Adapter, ScatterGather,
                                     Request->Header.Type == VIRTIO_BLK_T_OUT);
        Request->ScatterGather = NULL;
    }

    Physical = StorPortGetPhysicalAddress(Adapter, NULL, &Request->Status, &Length);
    if (Physical.QuadPart == 0 || Length < sizeof(Request->Status)) {
        DPrintf(0, "viostor: submit failed status-physical=%I64x length=%lu\n",
                Physical.QuadPart, Length);
        VioReleaseRequest(Adapter, Request);
        return FALSE;
    }
    Request->Sg[Request->SgCount].physAddr = Physical;
    Request->Sg[Request->SgCount].length = sizeof(Request->Status);
    Request->SgCount++;
    InCount++;

    Srb->SrbStatus = SRB_STATUS_PENDING;

    /*
     * Enqueue the request to the virtqueue and kick under InterruptLock.
     * This synchronizes with VioHwInterrupt (which runs at DIRQL with the
     * interrupt spinlock held) so the descriptor ring's free list is
     * never modified concurrently by StartIo and the ISR.
     */
    {
        STOR_LOCK_HANDLE LockHandle;

        StorPortAcquireSpinLock(Adapter, InterruptLock, NULL, &LockHandle);
        QueueAdded = virtqueue_add_buf(Adapter->Queue, Request->Sg, OutCount,
                                       InCount, Request, NULL, 0) == 0;
        if (QueueAdded) {
            virtqueue_kick_always(Adapter->Queue);
        }
        StorPortReleaseSpinLock(Adapter, &LockHandle);
    }

    if (!QueueAdded) {
        DPrintf(0, "viostor: submit failed queue-add sg=%lu out=%lu in=%lu\n",
                Request->SgCount, OutCount, InCount);
        VioReleaseRequest(Adapter, Request);
        return FALSE;
    }
    DPrintf(1, "viostor: queued opcode=0x%02x type=%u sector=%I64u bytes=%lu sg=%lu out=%lu in=%lu request=%p header=%I64x status=%I64x\n",
            Srb->Cdb[0], Type, Sector, Srb->DataTransferLength,
            Request->SgCount, OutCount, InCount, Request,
            Request->Sg[0].physAddr.QuadPart,
            Request->Sg[Request->SgCount - 1].physAddr.QuadPart);
    return TRUE;
}

static BOOLEAN VioInitializeDevice(PVIOSTOR_ADAPTER_EXTENSION Adapter)
{
    NTSTATUS Status;
    ULONGLONG Features;
    const char *Failure;

    Adapter->Initialized = FALSE;
    Adapter->Queue = NULL;
    Failure = "none";
    DPrintf(0, "viostor: init vendor=%04x device=%04x pci_length=%lu io_base=%p io_length=%lu\n",
            Adapter->PciConfig.VendorID, Adapter->PciConfig.DeviceID,
            Adapter->PciConfigLength, Adapter->IoBase, Adapter->IoLength);
    Status = virtio_device_initialize(&Adapter->Device, &VioSystemOps, Adapter, FALSE);
    DPrintf(0, "viostor: virtio_device_initialize status=0x%lx\n", Status);
    if (!NT_SUCCESS(Status)) {
        Failure = "virtio-device-initialize";
        goto Failure;
    }
    Features = virtio_get_features(&Adapter->Device);
    DPrintf(0, "viostor: host_features=0x%I64x\n", Features);
    /* This driver submits direct descriptors; do not negotiate INDIRECT_DESC. */
    Features &= (1ULL << VIRTIO_BLK_F_FLUSH) |
                (1ULL << VIRTIO_BLK_F_CONFIG_WCE) |
                (1ULL << VIRTIO_BLK_F_SEG_MAX) |
                (1ULL << VIRTIO_BLK_F_SIZE_MAX) |
                (1ULL << VIRTIO_BLK_F_RO);
    Status = virtio_set_features(&Adapter->Device, Features);
    DPrintf(0, "viostor: feature_negotiation requested=0x%I64x status=0x%lx\n",
            Features, Status);
    if (!NT_SUCCESS(Status)) {
        Failure = "feature-negotiation";
        goto Failure;
    }
    Adapter->Features = Features;
    Status = virtio_find_queue(&Adapter->Device, VIOSTOR_QUEUE_INDEX,
                               &Adapter->Queue);
    DPrintf(0, "viostor: queue_discovery index=%u queue=%p status=0x%lx\n",
            VIOSTOR_QUEUE_INDEX, Adapter->Queue, Status);
    if (!NT_SUCCESS(Status)) {
        Failure = "queue-discovery";
        goto Failure;
    }
    virtio_get_config(&Adapter->Device, 0, &Adapter->Config, sizeof(Adapter->Config));
    DPrintf(0, "viostor: block_config capacity=%I64x block_size=%lu seg_max=%lu size_max=%lu\n",
            Adapter->Config.Capacity, Adapter->Config.BlockSize,
            Adapter->Config.SegMax, Adapter->Config.SizeMax);
    if (Adapter->Config.BlockSize == 0)
        Adapter->Config.BlockSize = VIOSTOR_SECTOR_SIZE;
    if (Adapter->Config.BlockSize != VIOSTOR_SECTOR_SIZE ||
        Adapter->Config.Capacity == 0) {
        Status = STATUS_INVALID_PARAMETER;
        Failure = "block-config";
        goto Failure;
    }
    if (virtio_is_feature_enabled(Features, VIRTIO_BLK_F_SEG_MAX) &&
        Adapter->Config.SegMax == 0) {
        Status = STATUS_INVALID_PARAMETER;
        Failure = "seg-max";
        goto Failure;
    }
    if (virtio_is_feature_enabled(Features, VIRTIO_BLK_F_SIZE_MAX) &&
        (Adapter->Config.SizeMax == 0 ||
         Adapter->Config.SizeMax < PAGE_SIZE)) {
        Status = STATUS_INVALID_PARAMETER;
        Failure = "size-max";
        goto Failure;
    }
    {
        ULONG SegmentBytes = PAGE_SIZE;
        ULONG MaxBreaks = VIOSTOR_MAX_SG - 2u;

        if (virtio_is_feature_enabled(Features, VIRTIO_BLK_F_SEG_MAX))
            MaxBreaks = min(MaxBreaks, Adapter->Config.SegMax);
        if (virtio_is_feature_enabled(Features, VIRTIO_BLK_F_SIZE_MAX))
            SegmentBytes = min(SegmentBytes, Adapter->Config.SizeMax);
        Adapter->NumberOfPhysicalBreaks = MaxBreaks;
        Adapter->MaximumTransferLength =
            min(1024u * 1024u, MaxBreaks * SegmentBytes);
        Adapter->MaximumTransferLength &= ~(VIOSTOR_SECTOR_SIZE - 1u);
        if (Adapter->MaximumTransferLength < VIOSTOR_SECTOR_SIZE) {
            Status = STATUS_INVALID_PARAMETER;
            Failure = "transfer-limit";
            goto Failure;
        }
        if (Adapter->PortConfig != NULL) {
            Adapter->PortConfig->MaximumTransferLength =
                Adapter->MaximumTransferLength;
            Adapter->PortConfig->NumberOfPhysicalBreaks = MaxBreaks;
            Adapter->PortConfig->CachesData =
                virtio_is_feature_enabled(Features, VIRTIO_BLK_F_FLUSH);
        }
    }
    Adapter->LastLba = Adapter->Config.Capacity - 1;
    /*
     * Build a per-device unit serial from the PCI location and capacity.
     * The virtio-blk device config carries no serial field (a host-provided
     * serial would need a VIRTIO_BLK_T_GET_ID request, which this reduced
     * driver does not implement), so derive a stable unique-enough string
     * instead of reporting one fixed serial for every disk.
     */
    RtlStringCchPrintfA((PCHAR)Adapter->Serial,
                        sizeof(Adapter->Serial),
                        "VIOSTOR-%02lx-%02lx-%08lx",
                        Adapter->SystemIoBusNumber,
                        Adapter->SlotNumber,
                        (ULONG)(Adapter->Config.Capacity & 0xFFFFFFFFu));
    virtio_device_ready(&Adapter->Device);
    Adapter->Initialized = TRUE;
    DPrintf(0, "viostor: init success last_lba=%I64x max_transfer=%lu breaks=%lu\n",
            Adapter->LastLba, Adapter->MaximumTransferLength,
            Adapter->NumberOfPhysicalBreaks);
    return TRUE;

Failure:
    DPrintf(0, "viostor: init failure reason=%s status=0x%lx queue=%p\n",
            Failure, Status, Adapter->Queue);
    virtio_device_reset(&Adapter->Device);
    if (Adapter->Queue != NULL) {
        virtio_delete_queue(Adapter->Queue);
        Adapter->Queue = NULL;
    }
    return FALSE;
}
static BOOLEAN NTAPI VioHwInitialize(PVOID DeviceExtension)
{
    return VioInitializeDevice((PVIOSTOR_ADAPTER_EXTENSION)DeviceExtension);
}


static BOOLEAN NTAPI VioHwStartIo(PVOID DeviceExtension,
                                  PSCSI_REQUEST_BLOCK Srb)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = DeviceExtension;
    DPrintf(1, "viostor: startio function=0x%02x path=%u target=%u lun=%u opcode=0x%02x cdb1=0x%02x cdb2=0x%02x cdb4=0x%02x length=%lu\n",
            Srb->Function, Srb->PathId, Srb->TargetId, Srb->Lun,
            Srb->Cdb[0], Srb->Cdb[1], Srb->Cdb[2], Srb->Cdb[4],
            Srb->DataTransferLength);

    if (Srb->PathId != 0 || Srb->TargetId != 0 || Srb->Lun != 0) {
        VioComplete(Adapter, Srb, SRB_STATUS_INVALID_LUN);
        return TRUE;
    }
    if (Srb->Function == SRB_FUNCTION_CLAIM_DEVICE) {
        VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
        return TRUE;
    }
    if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {
        if (VioLocalCommand(Adapter, Srb))
            return TRUE;
    } else if (Srb->Function == SRB_FUNCTION_FLUSH ||
               Srb->Function == SRB_FUNCTION_SHUTDOWN) {
        if (Srb->DataTransferLength != 0 ||
            (Srb->Function == SRB_FUNCTION_FLUSH &&
             !virtio_is_feature_enabled(Adapter->Features,
                                         VIRTIO_BLK_F_FLUSH))) {
            VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return TRUE;
        }
        if (Srb->Function == SRB_FUNCTION_SHUTDOWN &&
            !virtio_is_feature_enabled(Adapter->Features,
                                       VIRTIO_BLK_F_FLUSH)) {
            VioComplete(Adapter, Srb, SRB_STATUS_SUCCESS);
            return TRUE;
        }
    } else {
        VioComplete(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
        return TRUE;
    }

    if (!Adapter->Initialized || Adapter->Queue == NULL) {
        VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
        return TRUE;
    }
    if (!VioSubmitIo(Adapter, Srb)) {
        VioComplete(Adapter, Srb, SRB_STATUS_ERROR);
        return TRUE;
    }
    return TRUE;
}

static BOOLEAN NTAPI VioHwInterrupt(PVOID DeviceExtension)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = DeviceExtension;
    UCHAR InterruptStatus;
    unsigned int Length;
    PVOID Opaque;
    PVIOSTOR_SRB_EXTENSION Request;
    PSCSI_REQUEST_BLOCK Srb;
    UCHAR Status;
    BOOLEAN Handled;

    InterruptStatus = virtio_read_isr_status(&Adapter->Device);
    if ((InterruptStatus & 1) == 0) {
        /*
         * Some legacy PCI paths can deliver a shared interrupt after the
         * device has already consumed the queue status byte. Do not discard
         * a used-ring completion solely because the latched ISR bit is gone.
         */
        if (!Adapter->Initialized || Adapter->Queue == NULL ||
            !virtqueue_has_buf(Adapter->Queue))
            return FALSE;
        DPrintf(0, "viostor: used-ring completion without queue ISR status=0x%02x\n",
                InterruptStatus);
    }

    if (!Adapter->Initialized || Adapter->Queue == NULL) {
        return FALSE;
    }

    /*
     * Drain ALL pending used-ring entries and complete each one directly
     * via StorPortNotification(RequestComplete).  This is safe at DIRQL:
     * StorPortNotification pushes the context onto the lock-free SList
     * CompletionList and queues the port CompletionDpc, which runs at
     * DISPATCH_LEVEL and does the IRP completion.  No miniport DPC needed.
     */
    Handled = FALSE;
    while ((Opaque = virtqueue_get_buf(Adapter->Queue, &Length)) != NULL)
    {
        Request = (PVIOSTOR_SRB_EXTENSION)Opaque;
        Srb = Request->Srb;
        if (Srb == NULL)
            continue;
        DPrintf(1, "viostor: used opcode=0x%02x length=%lu virtio_status=0x%02x request=%p\n",
                Srb->Cdb[0], Length, Request->Status, Request);
        Status = (Request->Status == VIRTIO_BLK_S_OK) ?
                 SRB_STATUS_SUCCESS :
                 (Request->Status == VIRTIO_BLK_S_UNSUPP ?
                  SRB_STATUS_INVALID_REQUEST : SRB_STATUS_ERROR);
        VioReleaseRequest(Adapter, Request);
        VioComplete(Adapter, Srb, Status);
        Handled = TRUE;
    }

    return Handled;
}

static BOOLEAN NTAPI VioHwResetBus(PVOID DeviceExtension, ULONG PathId)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = DeviceExtension;
    KIRQL Irql;
    BOOLEAN WasInitialized;

    UNREFERENCED_PARAMETER(PathId);
    KeAcquireSpinLock(&Adapter->Lock, &Irql);
    WasInitialized = Adapter->Initialized;
    Adapter->Initialized = FALSE;
    if (WasInitialized) {
        virtio_device_reset(&Adapter->Device);
        if (Adapter->Queue != NULL) {
            virtio_delete_queue(Adapter->Queue);
            Adapter->Queue = NULL;
        }
    }
    KeReleaseSpinLock(&Adapter->Lock, Irql);

    if (!WasInitialized)
        return TRUE;
    return VioInitializeDevice(Adapter);
}

static ULONG NTAPI VioHwFindAdapter(PVOID DeviceExtension, PVOID HwContext,
                                    PVOID BusInformation, PCHAR ArgumentString,
                                    PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                                    PBOOLEAN Again)
{
    PVIOSTOR_ADAPTER_EXTENSION Adapter = DeviceExtension;
    UCHAR ConfigBuffer[sizeof(PCI_COMMON_CONFIG)];
    PPCI_COMMON_CONFIG PciConfig;
    PACCESS_RANGE Ranges;
    ULONG PciLength;
    ULONG Index;

    UNREFERENCED_PARAMETER(HwContext);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ArgumentString);
    *Again = FALSE;
    DPrintf(0, "viostor: find bus=%lu slot=%lu ranges=%lu access=%p\n",
            ConfigInfo->SystemIoBusNumber, ConfigInfo->SlotNumber,
            ConfigInfo->NumberOfAccessRanges, ConfigInfo->AccessRanges);
    RtlZeroMemory(Adapter, sizeof(*Adapter));
    Adapter->PortConfig = ConfigInfo;
    KeInitializeSpinLock(&Adapter->Lock);
    Adapter->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;
    Adapter->SlotNumber = ConfigInfo->SlotNumber;
    RtlZeroMemory(ConfigBuffer, sizeof(ConfigBuffer));
    PciLength = StorPortGetBusData(Adapter, PCIConfiguration,
                                   Adapter->SystemIoBusNumber, Adapter->SlotNumber,
                                   ConfigBuffer, sizeof(ConfigBuffer));
    PciConfig = (PPCI_COMMON_CONFIG)ConfigBuffer;
    DPrintf(0, "viostor: pci_config length=%lu vendor=%04x device=%04x\n",
            PciLength, PciConfig->VendorID, PciConfig->DeviceID);
    if (PciLength < FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses)) {
        DPrintf(0, "viostor: find return=%lu reason=pci-config-short\n",
                SP_RETURN_NOT_FOUND);
        return SP_RETURN_NOT_FOUND;
    }
    if (PciConfig->VendorID != VIOSTOR_VENDOR_ID ||
        PciConfig->DeviceID != VIOSTOR_DEVICE_ID) {
        DPrintf(0, "viostor: find return=%lu reason=pci-id vendor=%04x device=%04x\n",
                SP_RETURN_NOT_FOUND, PciConfig->VendorID, PciConfig->DeviceID);
        return SP_RETURN_NOT_FOUND;
    }
    /*
     * QEMU's virtio-blk on pc-i440fx is transitional: it has legacy device
     * ID 0x1001, a legacy I/O BAR, and modern capabilities. The legacy path
     * is usable; reject only devices without a legacy I/O BAR below.
     */
    RtlCopyMemory(&Adapter->PciConfig, PciConfig,
                  min(PciLength, (ULONG)sizeof(Adapter->PciConfig)));
    Adapter->PciConfigLength = min(PciLength, (ULONG)sizeof(Adapter->PciConfig));
    if (ConfigInfo->NumberOfAccessRanges == 0 ||
        ConfigInfo->AccessRanges == NULL) {
        DPrintf(0, "viostor: find return=%lu reason=no-access-ranges count=%lu access=%p\n",
                SP_RETURN_ERROR, ConfigInfo->NumberOfAccessRanges,
                ConfigInfo->AccessRanges);
        return SP_RETURN_ERROR;
    }
    Ranges = *(ConfigInfo->AccessRanges);
    if (Ranges == NULL) {
        DPrintf(0, "viostor: find return=%lu reason=null-access-ranges\n",
                SP_RETURN_ERROR);
        return SP_RETURN_ERROR;
    }
    for (Index = 0; Index < ConfigInfo->NumberOfAccessRanges; Index++) {
        DPrintf(0, "viostor: range[%lu] start=0x%I64x length=0x%lx memory=%u\n",
                Index, Ranges[Index].RangeStart.QuadPart,
                Ranges[Index].RangeLength, Ranges[Index].RangeInMemory);
        if (!Ranges[Index].RangeInMemory && Ranges[Index].RangeLength != 0) {
            Adapter->IoLength = Ranges[Index].RangeLength;
            Adapter->PortSpace = TRUE;
            Adapter->IoBase = StorPortGetDeviceBase(
                Adapter,
                ConfigInfo->AdapterInterfaceType,
                ConfigInfo->SystemIoBusNumber,
                Ranges[Index].RangeStart,
                Ranges[Index].RangeLength,
                TRUE);
            DPrintf(0, "viostor: io-map start=0x%I64x length=0x%lx base=%p\n",
                    Ranges[Index].RangeStart.QuadPart,
                    Ranges[Index].RangeLength, Adapter->IoBase);
            if (Adapter->IoBase == NULL) {
                DPrintf(0, "viostor: find return=%lu reason=io-map-failed\n",
                        SP_RETURN_ERROR);
                return SP_RETURN_ERROR;
            }
            break;
        }
    }
    if (Adapter->IoBase == NULL) {
        DPrintf(0, "viostor: find return=%lu reason=no-io-range\n",
                SP_RETURN_NOT_FOUND);
        return SP_RETURN_NOT_FOUND;
    }
    if (Adapter->IoLength < VIRTIO_PCI_CONFIG(0) +
                            sizeof(VIOSTOR_BLK_CONFIG)) {
        DPrintf(0, "viostor: find return=%lu reason=io-range-short length=0x%lx required=0x%lx\n",
                SP_RETURN_NOT_FOUND, Adapter->IoLength,
                VIRTIO_PCI_CONFIG(0) + (ULONG)sizeof(VIOSTOR_BLK_CONFIG));
        return SP_RETURN_NOT_FOUND;
    }
    Adapter->QueueMemory = StorPortGetUncachedExtension(Adapter, ConfigInfo,
                                                         VIOSTOR_QUEUE_MEMORY_SIZE);
    DPrintf(0, "viostor: queue-memory size=0x%lx result=%p\n",
            VIOSTOR_QUEUE_MEMORY_SIZE, Adapter->QueueMemory);
    if (Adapter->QueueMemory == NULL) {
        DPrintf(0, "viostor: find return=%lu reason=queue-memory\n",
                SP_RETURN_ERROR);
        return SP_RETURN_ERROR;
    }
    ConfigInfo->DeviceExtensionSize = sizeof(*Adapter);
    ConfigInfo->SrbExtensionSize = sizeof(VIOSTOR_SRB_EXTENSION);
    ConfigInfo->MaximumNumberOfTargets = 1;
    ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->NeedPhysicalAddresses = TRUE;
    ConfigInfo->MapBuffers = TRUE;
    ConfigInfo->TaggedQueuing = FALSE;
    ConfigInfo->MultipleRequestPerLu = TRUE;
    ConfigInfo->CachesData = FALSE;
    Adapter->MaximumTransferLength = VIOSTOR_MAX_TRANSFER_LENGTH;
    Adapter->NumberOfPhysicalBreaks = VIOSTOR_MAX_SG - 2u;
    ConfigInfo->MaximumTransferLength = Adapter->MaximumTransferLength;
    ConfigInfo->NumberOfPhysicalBreaks = Adapter->NumberOfPhysicalBreaks;
    DPrintf(0, "viostor: find return=%lu success io_base=%p io_length=0x%lx queue-memory=%p\n",
            SP_RETURN_FOUND, Adapter->IoBase, Adapter->IoLength,
            Adapter->QueueMemory);
    return SP_RETURN_FOUND;
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject,
                           PUNICODE_STRING RegistryPath)
{
    HW_INITIALIZATION_DATA InitData;
    UNREFERENCED_PARAMETER(DriverObject);
    DPrintf(0, "viostor: driverentry\n", 0);
    RtlZeroMemory(&InitData, sizeof(InitData));
    InitData.HwInitializationDataSize = sizeof(InitData);
    InitData.AdapterInterfaceType = PCIBus;
    InitData.HwInitialize = VioHwInitialize;
    InitData.HwStartIo = VioHwStartIo;
    InitData.HwInterrupt = VioHwInterrupt;
    InitData.HwFindAdapter = VioHwFindAdapter;
    InitData.HwResetBus = VioHwResetBus;
    InitData.DeviceExtensionSize = sizeof(VIOSTOR_ADAPTER_EXTENSION);
    InitData.SrbExtensionSize = sizeof(VIOSTOR_SRB_EXTENSION);
    InitData.NumberOfAccessRanges = 1;
    InitData.NeedPhysicalAddresses = TRUE;
    InitData.MapBuffers = TRUE;
    InitData.MultipleRequestPerLu = TRUE;
    return StorPortInitialize(DriverObject, RegistryPath, &InitData, NULL);
}

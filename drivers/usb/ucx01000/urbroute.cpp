/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USBPORT style URB validation and routing
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"
#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

/* Status mapping, shared by the whole driver */

NTSTATUS
NTAPI
UcxUsbdStatusToNtStatus(
    _In_ USBD_STATUS UsbdStatus)
{
    switch (UsbdStatus)
    {
        case USBD_STATUS_SUCCESS:
        case USBD_STATUS_PORT_OPERATION_PENDING:
            return STATUS_SUCCESS;

        case USBD_STATUS_INSUFFICIENT_RESOURCES:
            return STATUS_INSUFFICIENT_RESOURCES;

        case USBD_STATUS_INVALID_URB_FUNCTION:
        case USBD_STATUS_INVALID_PARAMETER:
        case USBD_STATUS_INVALID_PIPE_HANDLE:
        case USBD_STATUS_BAD_START_FRAME:
            return STATUS_INVALID_PARAMETER;

        case USBD_STATUS_NOT_SUPPORTED:
            return STATUS_NOT_SUPPORTED;

        case USBD_STATUS_DEVICE_GONE:
            return STATUS_NO_SUCH_DEVICE;

        case USBD_STATUS_CANCELED:
            return STATUS_CANCELLED;

        default:
            return STATUS_UNSUCCESSFUL;
    }
}

/* Any other code, success codes such as STATUS_PENDING included, becomes INVALID_PARAMETER */
USBD_STATUS
NTAPI
UcxNtStatusToUsbdStatus(
    _In_ NTSTATUS Status)
{
    switch (Status)
    {
        case STATUS_SUCCESS:
            return USBD_STATUS_SUCCESS;

        case STATUS_NO_SUCH_DEVICE:
            return USBD_STATUS_DEVICE_GONE;

        case STATUS_INSUFFICIENT_RESOURCES:
            return USBD_STATUS_INSUFFICIENT_RESOURCES;

        case STATUS_NOT_SUPPORTED:
            return USBD_STATUS_NOT_SUPPORTED;

        case STATUS_CANCELLED:
            return USBD_STATUS_CANCELED;

        default:
            return USBD_STATUS_INVALID_PARAMETER;
    }
}

/* Buffers handed over without an MDL */

VOID
NTAPI
UcxLockTransferBuffer(
    _Inout_ PURB Urb)
{
    struct _URB_CONTROL_TRANSFER* Transfer = &Urb->UrbControlTransfer;
    LOCK_OPERATION Operation;
    PMDL Mdl;

    Urb->UrbHeader.UsbdFlags &= ~UCX_URB_FLAG_BUFFER_LOCKED;

    /* At DISPATCH_LEVEL the buffer has to be resident already */
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL ||
        Transfer->TransferBufferMDL != NULL ||
        Transfer->TransferBuffer == NULL ||
        Transfer->TransferBufferLength == 0)
    {
        return;
    }

    Mdl = IoAllocateMdl(Transfer->TransferBuffer, Transfer->TransferBufferLength, FALSE, FALSE, NULL);
    if (Mdl == NULL)
    {
        DPRINT1("No MDL for the %lu byte buffer of URB %p\n", Transfer->TransferBufferLength, Urb);
        return;
    }

    Operation = (Transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN) ? IoWriteAccess : IoReadAccess;

    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, KernelMode, Operation);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Locking the buffer of URB %p failed 0x%lx\n", Urb, _SEH2_GetExceptionCode());
        IoFreeMdl(Mdl);
        Mdl = NULL;
    }
    _SEH2_END;

    if (Mdl == NULL)
        return;

    Transfer->TransferBufferMDL = Mdl;
    Urb->UrbHeader.UsbdFlags |= UCX_URB_FLAG_BUFFER_LOCKED;
}

VOID
NTAPI
UcxUnlockTransferBuffer(
    _Inout_ PURB Urb)
{
    struct _URB_CONTROL_TRANSFER* Transfer = &Urb->UrbControlTransfer;

    if (!(Urb->UrbHeader.UsbdFlags & UCX_URB_FLAG_BUFFER_LOCKED))
        return;

    MmUnlockPages(Transfer->TransferBufferMDL);
    IoFreeMdl(Transfer->TransferBufferMDL);
    Transfer->TransferBufferMDL = NULL;
    Urb->UrbHeader.UsbdFlags &= ~UCX_URB_FLAG_BUFFER_LOCKED;
}

/* The function table */

enum class UcxUrbHandler : UCHAR
{
    NotSupported,
    InvalidFunction,
    AbortPipe,
    GetCurrentFrame,
    ControlTransfer,
    BulkOrInterrupt,
    Isoch,
    GetSetDescriptor,
    SetClearFeature,
    GetStatus,
    VendorClass,
    GetConfiguration,
    GetInterface,
    MsFeatureDescriptor
};

/* Table flags; never written into the URB */
#define UCX_URB_TRANSFER        0x01
#define UCX_URB_DEFAULT_PIPE    0x02
#define UCX_URB_NO_DATA         0x04
#define UCX_URB_PIPE_REQUEST    0x08

/* Setup packet fields the table supplies; Dir 2 means the client's direction bit decides */
#define UCX_DIR_OUT     0
#define UCX_DIR_IN      1
#define UCX_DIR_CLIENT  2

struct UcxUrbFunction
{
    UcxUrbHandler Handler;
    USHORT Length;
    UCHAR Flags;
    UCHAR Direction;
    UCHAR Type;
    UCHAR Recipient;
    UCHAR Request;
};

#define UCX_RT_STANDARD  0
#define UCX_RT_CLASS     1
#define UCX_RT_VENDOR    2

#define UCX_TO_DEVICE     0
#define UCX_TO_INTERFACE  1
#define UCX_TO_ENDPOINT   2
#define UCX_TO_OTHER      3

#define UCX_PIPE_REQUEST_SIZE   sizeof(struct _URB_PIPE_REQUEST)
#define UCX_DESCRIPTOR_SIZE     sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST)
#define UCX_FEATURE_SIZE        sizeof(struct _URB_CONTROL_FEATURE_REQUEST)
#define UCX_STATUS_SIZE         sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST)
#define UCX_VENDOR_SIZE         sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST)

#define UCX_STD_TRANSFER        (UCX_URB_TRANSFER | UCX_URB_DEFAULT_PIPE)
#define UCX_STD_NO_DATA         (UCX_URB_TRANSFER | UCX_URB_DEFAULT_PIPE | UCX_URB_NO_DATA)

#define UCX_UNSUPPORTED(Length)        { UcxUrbHandler::NotSupported, (USHORT)(Length), 0, 0, 0, 0, 0 }
#define UCX_INVALID                    { UcxUrbHandler::InvalidFunction, 0, 0, 0, 0, 0, 0 }
#define UCX_DESCRIPTOR(Dir, To, Req)   { UcxUrbHandler::GetSetDescriptor, UCX_DESCRIPTOR_SIZE, UCX_STD_TRANSFER, Dir, UCX_RT_STANDARD, To, Req }
#define UCX_FEATURE(To, Req)           { UcxUrbHandler::SetClearFeature, UCX_FEATURE_SIZE, UCX_STD_NO_DATA, UCX_DIR_OUT, UCX_RT_STANDARD, To, Req }
#define UCX_STATUS(To)                 { UcxUrbHandler::GetStatus, UCX_STATUS_SIZE, UCX_STD_TRANSFER, UCX_DIR_IN, UCX_RT_STANDARD, To, USB_REQUEST_GET_STATUS }
#define UCX_VENDOR_CLASS(Type, To)     { UcxUrbHandler::VendorClass, UCX_VENDOR_SIZE, UCX_STD_TRANSFER, UCX_DIR_CLIENT, Type, To, 0 }

static const UcxUrbFunction UcxUrbFunctions[] =
{
    /* 0x00 */ UCX_UNSUPPORTED(0),
    /* 0x01 */ UCX_UNSUPPORTED(0),
    /* 0x02 */ { UcxUrbHandler::AbortPipe, UCX_PIPE_REQUEST_SIZE, UCX_URB_PIPE_REQUEST, 0, 0, 0, 0 },
    /* 0x03 */ UCX_UNSUPPORTED(sizeof(struct _URB_FRAME_LENGTH_CONTROL)),
    /* 0x04 */ UCX_UNSUPPORTED(sizeof(struct _URB_FRAME_LENGTH_CONTROL)),
    /* 0x05 */ UCX_UNSUPPORTED(sizeof(struct _URB_GET_FRAME_LENGTH)),
    /* 0x06 */ UCX_UNSUPPORTED(sizeof(struct _URB_SET_FRAME_LENGTH)),
    /* 0x07 */ { UcxUrbHandler::GetCurrentFrame, sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER), 0, 0, 0, 0, 0 },
    /* 0x08 */ { UcxUrbHandler::ControlTransfer, sizeof(struct _URB_CONTROL_TRANSFER), UCX_URB_TRANSFER, 0, 0, 0, 0 },
    /* 0x09 */ { UcxUrbHandler::BulkOrInterrupt, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER), UCX_URB_TRANSFER, 0, 0, 0, 0 },
    /* 0x0A */ { UcxUrbHandler::Isoch, 0, UCX_URB_TRANSFER, 0, 0, 0, 0 },
    /* 0x0B */ UCX_DESCRIPTOR(UCX_DIR_IN, UCX_TO_DEVICE, USB_REQUEST_GET_DESCRIPTOR),
    /* 0x0C */ UCX_DESCRIPTOR(UCX_DIR_OUT, UCX_TO_DEVICE, USB_REQUEST_SET_DESCRIPTOR),
    /* 0x0D */ UCX_FEATURE(UCX_TO_DEVICE, USB_REQUEST_SET_FEATURE),
    /* 0x0E */ UCX_FEATURE(UCX_TO_INTERFACE, USB_REQUEST_SET_FEATURE),
    /* 0x0F */ UCX_FEATURE(UCX_TO_ENDPOINT, USB_REQUEST_SET_FEATURE),
    /* 0x10 */ UCX_FEATURE(UCX_TO_DEVICE, USB_REQUEST_CLEAR_FEATURE),
    /* 0x11 */ UCX_FEATURE(UCX_TO_INTERFACE, USB_REQUEST_CLEAR_FEATURE),
    /* 0x12 */ UCX_FEATURE(UCX_TO_ENDPOINT, USB_REQUEST_CLEAR_FEATURE),
    /* 0x13 */ UCX_STATUS(UCX_TO_DEVICE),
    /* 0x14 */ UCX_STATUS(UCX_TO_INTERFACE),
    /* 0x15 */ UCX_STATUS(UCX_TO_ENDPOINT),
    /* 0x16 */ UCX_INVALID,
    /* 0x17 */ UCX_VENDOR_CLASS(UCX_RT_VENDOR, UCX_TO_DEVICE),
    /* 0x18 */ UCX_VENDOR_CLASS(UCX_RT_VENDOR, UCX_TO_INTERFACE),
    /* 0x19 */ UCX_VENDOR_CLASS(UCX_RT_VENDOR, UCX_TO_ENDPOINT),
    /* 0x1A */ UCX_VENDOR_CLASS(UCX_RT_CLASS, UCX_TO_DEVICE),
    /* 0x1B */ UCX_VENDOR_CLASS(UCX_RT_CLASS, UCX_TO_INTERFACE),
    /* 0x1C */ UCX_VENDOR_CLASS(UCX_RT_CLASS, UCX_TO_ENDPOINT),
    /* 0x1D */ UCX_INVALID,
    /* 0x1E */ { UcxUrbHandler::NotSupported, UCX_PIPE_REQUEST_SIZE, UCX_URB_PIPE_REQUEST, 0, 0, 0, 0 },
    /* 0x1F */ UCX_VENDOR_CLASS(UCX_RT_CLASS, UCX_TO_OTHER),
    /* 0x20 */ UCX_VENDOR_CLASS(UCX_RT_VENDOR, UCX_TO_OTHER),
    /* 0x21 */ UCX_STATUS(UCX_TO_OTHER),
    /* 0x22 */ UCX_FEATURE(UCX_TO_OTHER, USB_REQUEST_CLEAR_FEATURE),
    /* 0x23 */ UCX_FEATURE(UCX_TO_OTHER, USB_REQUEST_SET_FEATURE),
    /* 0x24 */ UCX_DESCRIPTOR(UCX_DIR_IN, UCX_TO_ENDPOINT, USB_REQUEST_GET_DESCRIPTOR),
    /* 0x25 */ UCX_DESCRIPTOR(UCX_DIR_OUT, UCX_TO_ENDPOINT, USB_REQUEST_SET_DESCRIPTOR),
    /* 0x26 */ { UcxUrbHandler::GetConfiguration, sizeof(struct _URB_CONTROL_GET_CONFIGURATION_REQUEST), UCX_STD_TRANSFER,
                 UCX_DIR_IN, UCX_RT_STANDARD, UCX_TO_DEVICE, USB_REQUEST_GET_CONFIGURATION },
    /* 0x27 */ { UcxUrbHandler::GetInterface, sizeof(struct _URB_CONTROL_GET_INTERFACE_REQUEST), UCX_STD_TRANSFER,
                 UCX_DIR_IN, UCX_RT_STANDARD, UCX_TO_INTERFACE, USB_REQUEST_GET_INTERFACE },
    /* 0x28 */ UCX_DESCRIPTOR(UCX_DIR_IN, UCX_TO_INTERFACE, USB_REQUEST_GET_DESCRIPTOR),
    /* 0x29 */ UCX_DESCRIPTOR(UCX_DIR_OUT, UCX_TO_INTERFACE, USB_REQUEST_SET_DESCRIPTOR),
    /* 0x2A */ { UcxUrbHandler::MsFeatureDescriptor, UCX_DESCRIPTOR_SIZE, UCX_STD_TRANSFER, UCX_DIR_IN, UCX_RT_VENDOR, 0, 0 },
    /* 0x2B */ UCX_INVALID,
    /* 0x2C */ UCX_INVALID,
    /* 0x2D */ UCX_INVALID,
    /* 0x2E */ UCX_INVALID,
    /* 0x2F */ UCX_INVALID,
    /* 0x30 */ { UcxUrbHandler::NotSupported, UCX_PIPE_REQUEST_SIZE, UCX_URB_PIPE_REQUEST, 0, 0, 0, 0 },
    /* 0x31 */ { UcxUrbHandler::NotSupported, UCX_PIPE_REQUEST_SIZE, UCX_URB_PIPE_REQUEST, 0, 0, 0, 0 },
    /* 0x32 */ { UcxUrbHandler::ControlTransfer, sizeof(struct _URB_CONTROL_TRANSFER_EX), UCX_URB_TRANSFER, 0, 0, 0, 0 },
    /* 0x33 */ UCX_INVALID,
    /* 0x34 */ UCX_INVALID,
};

C_ASSERT(ARRAYSIZE(UcxUrbFunctions) == URB_FUNCTION_RESERVE_0X0034 + 1);

static IO_COMPLETION_ROUTINE UcxLegacyTransferCompletion;

/* Forwarding */

/** Keeps the PDO's driver loaded until the routine ran when the system allows it. */
static
VOID
NTAPI
UcxSetCompletionOnNext(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE Completion,
    _In_ PVOID Context)
{
    NTSTATUS Status;

    IoMarkIrpPending(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    Status = IoSetCompletionRoutineEx(WdfDeviceWdmGetDeviceObject(RootHubPdo),
                                      Irp,
                                      Completion,
                                      Context,
                                      TRUE,
                                      TRUE,
                                      TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoSetCompletionRoutineEx failed 0x%lx for IRP %p, using the plain routine\n", Status, Irp);
        IoSetCompletionRoutine(Irp, Completion, Context, TRUE, TRUE, TRUE);
    }
}

/* A refused IRP still runs the completion routine, which cancels the URB */
NTSTATUS
NTAPI
UcxForwardIrpToQueue(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE Completion,
    _In_ PVOID Context,
    _In_ WDFQUEUE Queue)
{
    UcxSetCompletionOnNext(RootHubPdo, Irp, Completion, Context);

    WdfDeviceWdmDispatchIrpToIoQueue(RootHubPdo, Irp, Queue, WDF_DISPATCH_IRP_TO_IO_QUEUE_PREPROCESSED_IRP);
    return STATUS_PENDING;
}

NTSTATUS
NTAPI
UcxForwardIrpWithRequest(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE Completion,
    _In_ PVOID Context,
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_FORWARD_OPTIONS Options;
    NTSTATUS Status;

    UcxSetCompletionOnNext(RootHubPdo, Irp, Completion, Context);
    IoSetNextIrpStackLocation(Irp);

    WDF_REQUEST_FORWARD_OPTIONS_INIT(&Options);
    Status = WdfRequestForwardToParentDeviceIoQueue(Request, Queue, &Options);
    if (!NT_SUCCESS(Status))
    {
        /* A purging queue means a controller reset or a concurrent pipe reset */
        if (Status == STATUS_WDF_BUSY)
            Status = STATUS_NO_SUCH_DEVICE;

        DPRINT1("Forwarding request %p (IRP %p) to queue %p failed 0x%lx\n", Request, Irp, Queue, Status);
        WdfRequestComplete(Request, Status);
    }

    return STATUS_PENDING;
}

/* Pipe handles */

BOOLEAN
NTAPI
UcxValidatePipeHandle(
    _In_ UcxUsbDevice* Device,
    _In_ USBD_PIPE_HANDLE Handle)
{
    UcxPipe* Pipe = UcxPipe::FromHandle(Handle);
    BOOLEAN Valid = FALSE;
    PLIST_ENTRY Entry;

    if (Pipe != NULL && !Pipe->BelongsToStream)
    {
        if (Device->m_StalePipeIoAllowed)
        {
            SpinLockGuard Guard(&Device->m_TrackingLock);

            for (Entry = Device->m_TrackingList.Flink; Entry != &Device->m_TrackingList; Entry = Entry->Flink)
            {
                if (&CONTAINING_RECORD(Entry, UcxEndpoint, m_TrackingLink)->m_Pipe == Pipe)
                {
                    Valid = TRUE;
                    break;
                }
            }
        }
        else
        {
            Valid = Pipe->IsValid();
        }
    }

    if (!Valid && UcxAnyUsbdHandleHasVerifier(Device))
    {
        DPRINT1("Client used invalid pipe handle %p on device %p\n", Handle, Device);
        if (!KdRefreshDebuggerNotPresent())
            DbgBreakPoint();
    }

    return Valid;
}

/** A single MDL is always accepted, even if shorter than the transfer. */
static
BOOLEAN
NTAPI
UcxChainedMdlMisaligned(
    _In_ PMDL Mdl,
    _In_ ULONG Length)
{
    ULONG Remaining;

    if (Mdl->Next == NULL || Length <= MmGetMdlByteCount(Mdl))
        return FALSE;

    if ((MmGetMdlByteOffset(Mdl) + MmGetMdlByteCount(Mdl)) % PAGE_SIZE != 0)
        return TRUE;

    Remaining = Length - MmGetMdlByteCount(Mdl);

    for (Mdl = Mdl->Next; Mdl != NULL; Mdl = Mdl->Next)
    {
        if (MmGetMdlByteOffset(Mdl) != 0)
            return TRUE;

        if (Remaining <= MmGetMdlByteCount(Mdl))
            return FALSE;

        if (MmGetMdlByteCount(Mdl) % PAGE_SIZE != 0)
            return TRUE;

        Remaining -= MmGetMdlByteCount(Mdl);
    }

    return TRUE;
}

/* Handlers */

static
PUCHAR
NTAPI
UcxSetupPacket(
    _In_ PURB Urb)
{
    return Urb->UrbControlTransfer.SetupPacket;
}

static
UCHAR
NTAPI
UcxRequestType(
    _In_ UCHAR Direction,
    _In_ UCHAR Type,
    _In_ UCHAR Recipient)
{
    return (UCHAR)((Direction << 7) | (Type << 5) | Recipient);
}

static
VOID
NTAPI
UcxSetWord(
    _Out_writes_bytes_(2) PUCHAR Where,
    _In_ USHORT Value)
{
    Where[0] = (UCHAR)(Value & 0xFF);
    Where[1] = (UCHAR)(Value >> 8);
}

static
USHORT
NTAPI
UcxGetWord(
    _In_reads_bytes_(2) const UCHAR* Where)
{
    return (USHORT)(Where[0] | (Where[1] << 8));
}

/* Default pipe requests only ever reach the HCD as plain control transfers, as with USBPORT */
static
NTSTATUS
NTAPI
UcxForwardDefaultPipeRequest(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ WDFQUEUE Queue)
{
    if (Urb->UrbControlTransfer.TransferFlags & USBD_DEFAULT_PIPE_TRANSFER)
        Urb->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;

    UcxLockTransferBuffer(Urb);
    return UcxForwardIrpToQueue(RootHubPdo, Irp, UcxLegacyTransferCompletion, Urb, Queue);
}

static
NTSTATUS
NTAPI
UcxHandleStandardRequest(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ WDFQUEUE Queue,
    _In_ const UcxUrbFunction* Function)
{
    PUCHAR Setup = UcxSetupPacket(Urb);
    PULONG Flags = &Urb->UrbControlTransfer.TransferFlags;
    USHORT Length;

    *Flags |= USBD_SHORT_TRANSFER_OK;

    switch (Function->Handler)
    {
        case UcxUrbHandler::GetSetDescriptor:
            Setup[0] = UcxRequestType(Function->Direction, Function->Type, Function->Recipient);
            Setup[1] = Function->Request;
            break;

        case UcxUrbHandler::SetClearFeature:
            Urb->UrbControlTransfer.TransferBufferLength = 0;
            Setup[0] = UcxRequestType(Function->Direction, Function->Type, Function->Recipient);
            Setup[1] = Function->Request;
            break;

        case UcxUrbHandler::GetStatus:
        case UcxUrbHandler::GetInterface:
            Setup[0] = UcxRequestType(Function->Direction, Function->Type, Function->Recipient);
            Setup[1] = Function->Request;
            UcxSetWord(&Setup[2], 0);
            break;

        case UcxUrbHandler::GetConfiguration:
            Setup[0] = UcxRequestType(Function->Direction, Function->Type, Function->Recipient);
            Setup[1] = Function->Request;
            UcxSetWord(&Setup[2], 0);
            UcxSetWord(&Setup[4], 0);
            break;

        /* The client's direction bit and reserved request type bits are kept */
        case UcxUrbHandler::VendorClass:
            Setup[0] = (UCHAR)(UcxRequestType((*Flags & USBD_TRANSFER_DIRECTION_IN) ? 1 : 0,
                                              Function->Type,
                                              Function->Recipient) |
                               (Setup[0] & 0x1C));
            break;

        /* The recipient comes from the client, the request code from the hub */
        case UcxUrbHandler::MsFeatureDescriptor:
            Setup[0] = (UCHAR)(UcxRequestType(UCX_DIR_IN, UCX_RT_VENDOR, 0) | (Setup[0] & 0x1F));
            break;

        default:
            NT_ASSERT(FALSE);
            break;
    }

    if (Function->Handler != UcxUrbHandler::VendorClass)
    {
        if (Function->Direction == UCX_DIR_IN)
            *Flags |= USBD_TRANSFER_DIRECTION_IN;
        else
            *Flags &= ~USBD_TRANSFER_DIRECTION_IN;
    }

    Length = (USHORT)Urb->UrbControlTransfer.TransferBufferLength;
    UcxSetWord(&Setup[6], Length);

    /* The setup bytes stay written even when this fails */
    if (Function->Handler == UcxUrbHandler::GetStatus && UcxGetWord(&Setup[6]) != sizeof(USHORT))
    {
        DPRINT1("GET_STATUS URB %p has wLength %u, expected 2\n", Urb, UcxGetWord(&Setup[6]));
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    return UcxForwardDefaultPipeRequest(RootHubPdo, Irp, Urb, Queue);
}

static
NTSTATUS
NTAPI
UcxHandleIsoch(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ WDFQUEUE Queue,
    _In_ UcxPipe* Pipe)
{
    struct _URB_ISOCH_TRANSFER* Isoch = &Urb->UrbIsochronousTransfer;
    ULONG Packets = Isoch->NumberOfPackets;

    if (Isoch->TransferBufferLength == 0 && Isoch->TransferBufferMDL == NULL && Isoch->TransferBuffer == NULL)
    {
        DPRINT1("Isoch URB %p has no buffer\n", Urb);
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    if (Pipe->IsochPeriodMicroframes > 8)
    {
        DPRINT1("Isoch URB %p on pipe %p has period %lu microframes\n", Urb, Pipe, Pipe->IsochPeriodMicroframes);
        Isoch->StartFrame = 0;
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    /* Only the frame alignment failure resets StartFrame */
    if (Packets != 0 && (Packets * Pipe->IsochPeriodMicroframes) % 8 != 0)
    {
        DPRINT1("Isoch URB %p: %lu packets do not fill whole frames\n", Urb, Packets);
        Isoch->StartFrame = 0;
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    if (Packets == 0 || Packets > Pipe->IsochPacketLimit)
    {
        DPRINT1("Isoch URB %p has bad packet count %lu, max %lu\n", Urb, Packets, Pipe->IsochPacketLimit);
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    UcxSetTransferDirection(Urb, Pipe);
    UcxLockTransferBuffer(Urb);
    return UcxForwardIrpToQueue(RootHubPdo, Irp, UcxLegacyTransferCompletion, Urb, Queue);
}

static
NTSTATUS
NTAPI
UcxHandleGetCurrentFrame(
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ UcxUsbDevice* Device)
{
    ULONG Frame = 0;
    NTSTATUS Status;

    Status = Device->m_Controller->GetCurrentFrameNumber(&Frame);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Getting the current frame for URB %p failed 0x%lx\n", Urb, Status);
        Frame = 0;
    }

    Urb->UrbGetCurrentFrameNumber.FrameNumber = Frame;
    return UcxCompleteUrb(Irp, Urb, Status, UcxNtStatusToUsbdStatus(Status));
}

/** The transfer specific checks; the last recorded error wins. */
static
USBD_STATUS
NTAPI
UcxCheckTransferAgainstPipe(
    _In_ PURB Urb,
    _In_ const UcxPipe* Pipe)
{
    ULONG Length = Urb->UrbControlTransfer.TransferBufferLength;
    BOOLEAN AllowLarge = (Urb->UrbHeader.UsbdFlags & UCXHUB_URB_FLAG_ALLOW_LARGE_TRANSFER) != 0;
    USBD_STATUS Error = USBD_STATUS_SUCCESS;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_CONTROL_TRANSFER:
        case URB_FUNCTION_CONTROL_TRANSFER_EX:
            if (Pipe->TransferType != UcxTransferType::Control)
                Error = USBD_STATUS_INVALID_PIPE_HANDLE;
            if ((Length > Pipe->MaximumTransferSize && !AllowLarge) || Length > UCX_MAX_CONTROL_TRANSFER)
                Error = USBD_STATUS_INVALID_PARAMETER;
            break;

        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
            if (Pipe->TransferType != UcxTransferType::Bulk && Pipe->TransferType != UcxTransferType::Interrupt)
                Error = USBD_STATUS_INVALID_PIPE_HANDLE;
            if (Length > Pipe->MaximumTransferSize)
                Error = USBD_STATUS_INVALID_PARAMETER;
            break;

        case URB_FUNCTION_ISOCH_TRANSFER:
            if (Pipe->TransferType != UcxTransferType::Isochronous)
                Error = USBD_STATUS_INVALID_PIPE_HANDLE;
            break;
    }

    return Error;
}

/** Returns USBD_STATUS_SUCCESS with the target pipe, or the status to fail with. */
static
USBD_STATUS
NTAPI
UcxPrepareTransfer(
    _In_ PURB Urb,
    _In_ UcxUsbDevice* Device,
    _In_ const UcxUrbFunction* Function,
    _Out_ UcxPipe** TargetPipe)
{
    struct _URB_CONTROL_TRANSFER* Transfer = &Urb->UrbControlTransfer;
    USHORT Code = Urb->UrbHeader.Function;
    UcxPipe* Pipe;
    USBD_STATUS Error;

    *TargetPipe = NULL;

    UcxStampProcessorNumber(Urb);

    if (Function->Flags & UCX_URB_DEFAULT_PIPE)
    {
        Transfer->PipeHandle = Device->m_DefaultPipe->Handle();
        Transfer->TransferFlags |= USBD_DEFAULT_PIPE_TRANSFER;
    }

    if ((Transfer->TransferFlags & USBD_DEFAULT_PIPE_TRANSFER) &&
        (Code == URB_FUNCTION_CONTROL_TRANSFER || Code == URB_FUNCTION_CONTROL_TRANSFER_EX))
    {
        Transfer->PipeHandle = Device->m_DefaultPipe->Handle();
    }

    /* UrbLink must be unused; the _EX form keeps its Timeout in the same slot */
    if (Transfer->UrbLink != NULL && Code != URB_FUNCTION_CONTROL_TRANSFER_EX)
        return USBD_STATUS_INVALID_PARAMETER;

    if (Function->Flags & UCX_URB_NO_DATA)
    {
        Transfer->TransferBuffer = NULL;
        Transfer->TransferBufferMDL = NULL;
        Transfer->TransferBufferLength = 0;
    }

    if (!UcxValidatePipeHandle(Device, Transfer->PipeHandle))
        return USBD_STATUS_INVALID_PIPE_HANDLE;

    Pipe = UcxPipe::FromHandle(Transfer->PipeHandle);

    if (Pipe->Kind == UcxEndpointKind::ZeroBandwidth)
        return USBD_STATUS_INVALID_PARAMETER;

    Error = UcxCheckTransferAgainstPipe(Urb, Pipe);
    if (Error != USBD_STATUS_SUCCESS)
        return Error;

    /* A buffer with zero length is accepted, a length with no buffer is not */
    if (Transfer->TransferBufferLength != 0 && Transfer->TransferBuffer == NULL && Transfer->TransferBufferMDL == NULL)
        return USBD_STATUS_INVALID_PARAMETER;

    if (Transfer->TransferBufferLength == 0 && Transfer->TransferBufferMDL != NULL)
        return USBD_STATUS_INVALID_PARAMETER;

    if (Pipe->TransferType == UcxTransferType::Control &&
        ((Transfer->TransferBufferLength > Pipe->MaximumTransferSize &&
          !(Urb->UrbHeader.UsbdFlags & UCXHUB_URB_FLAG_ALLOW_LARGE_TRANSFER)) ||
         Transfer->TransferBufferLength > UCX_MAX_CONTROL_TRANSFER))
    {
        return USBD_STATUS_INVALID_PARAMETER;
    }

    /* Only fatal under verifier; the HCD copes otherwise */
    if (Transfer->TransferBufferMDL != NULL &&
        UcxChainedMdlMisaligned(Transfer->TransferBufferMDL, Transfer->TransferBufferLength) &&
        Device->m_Controller->m_DriverVerifierEnabled)
    {
        DPRINT1("URB %p has an unaligned chained MDL %p\n", Urb, Transfer->TransferBufferMDL);
        KeBugCheckEx(0x144, 0x807, (ULONG_PTR)Transfer->TransferBufferMDL, (ULONG_PTR)Urb, 0);
    }

    *TargetPipe = Pipe;
    return USBD_STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UcxProcessLegacyUrb(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    const UcxUrbFunction* Function;
    UcxUsbDevice* Device;
    UcxPipe* Pipe = NULL;
    USBD_STATUS Error;
    USHORT Code;

    /* Short headers are not rejected */
    Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
    Code = Urb->UrbHeader.Function;
    Urb->UrbHeader.UsbdFlags &= UCX_URB_FLAGS_KEEP_HUB_BITS;

    if (Code >= ARRAYSIZE(UcxUrbFunctions))
    {
        DPRINT1("URB %p has unknown function 0x%x\n", Urb, Code);
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_URB_FUNCTION);
    }

    Function = &UcxUrbFunctions[Code];

    if (Function->Length != 0 && Function->Length != Urb->UrbHeader.Length)
    {
        DPRINT1("URB %p function 0x%x has length %u, expected %u\n", Urb, Code, Urb->UrbHeader.Length, Function->Length);
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    /* The hub fills in its UCXUSBDEVICE */
    if (Urb->UrbHeader.UsbdDeviceHandle == NULL)
    {
        DPRINT1("URB %p function 0x%x has no device handle\n", Urb, Code);
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Urb->UrbHeader.UsbdDeviceHandle);

    if (Device->m_Disconnected)
    {
        DPRINT("URB %p function 0x%x for disconnected device %p\n", Urb, Code, Device);
        return UcxCompleteUrb(Irp, Urb, STATUS_NO_SUCH_DEVICE, USBD_STATUS_DEVICE_GONE);
    }

    if (Function->Flags & UCX_URB_TRANSFER)
    {
        Error = UcxPrepareTransfer(Urb, Device, Function, &Pipe);
        if (Error != USBD_STATUS_SUCCESS)
        {
            DPRINT1("Transfer URB %p function 0x%x on device %p rejected, USBD status 0x%lx\n", Urb, Code, Device, Error);
            return UcxCompleteUrb(Irp, Urb, UcxUsbdStatusToNtStatus(Error), Error);
        }
    }
    else if (Function->Flags & UCX_URB_PIPE_REQUEST)
    {
        if (!UcxValidatePipeHandle(Device, Urb->UrbPipeRequest.PipeHandle))
        {
            DPRINT1("Pipe URB %p function 0x%x has invalid pipe %p\n", Urb, Code, Urb->UrbPipeRequest.PipeHandle);
            return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PIPE_HANDLE);
        }

        Pipe = UcxPipe::FromHandle(Urb->UrbPipeRequest.PipeHandle);

        /* Zero bandwidth pipes are not in the schedule; pipe requests on them just succeed */
        if (Pipe->Kind == UcxEndpointKind::ZeroBandwidth)
            return UcxCompleteUrb(Irp, Urb, STATUS_SUCCESS, USBD_STATUS_SUCCESS);
    }

    /* Tells the completion routine whether the HCD ever saw it */
    Urb->UrbHeader.Status = USBD_STATUS_PENDING;

    switch (Function->Handler)
    {
        case UcxUrbHandler::NotSupported:
            DPRINT1("URB %p function 0x%x is not supported\n", Urb, Code);
            NT_ASSERT(FALSE);
            return UcxCompleteUrb(Irp, Urb, STATUS_NOT_SUPPORTED, USBD_STATUS_NOT_SUPPORTED);

        case UcxUrbHandler::InvalidFunction:
            DPRINT1("URB %p has invalid function 0x%x\n", Urb, Code);
            return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_URB_FUNCTION);

        case UcxUrbHandler::AbortPipe:
            return Pipe->Endpoint->AbortPipe(Irp, Urb);

        case UcxUrbHandler::GetCurrentFrame:
            return UcxHandleGetCurrentFrame(Irp, Urb, Device);

        case UcxUrbHandler::ControlTransfer:
            UcxLockTransferBuffer(Urb);
            return UcxForwardIrpToQueue(RootHubPdo, Irp, UcxLegacyTransferCompletion, Urb, Pipe->Queue);

        case UcxUrbHandler::BulkOrInterrupt:
            UcxSetTransferDirection(Urb, Pipe);
            UcxLockTransferBuffer(Urb);
            return UcxForwardIrpToQueue(RootHubPdo, Irp, UcxLegacyTransferCompletion, Urb, Pipe->Queue);

        case UcxUrbHandler::Isoch:
            return UcxHandleIsoch(RootHubPdo, Irp, Urb, Pipe->Queue, Pipe);

        default:
            return UcxHandleStandardRequest(RootHubPdo, Irp, Urb, Pipe->Queue, Function);
    }
}

/* Completion of every transfer forwarded by this path */

static
NTSTATUS
NTAPI
UcxLegacyTransferCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PURB Urb = (PURB)Context;
    UcxUsbDevice* Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Urb->UrbHeader.UsbdDeviceHandle);
    USBD_STATUS UsbdStatus = Urb->UrbHeader.Status;
    NTSTATUS Status = Irp->IoStatus.Status;
    ULONG Index;

    UNREFERENCED_PARAMETER(DeviceObject);

    /* The byte count lives in the URB only */
    Irp->IoStatus.Information = 0;

    if (UsbdStatus == USBD_STATUS_PENDING)
    {
        /* The HCD never saw it: purged, canceled while queued or refused */
        DPRINT("URB %p function 0x%x canceled before reaching the HCD\n", Urb, Urb->UrbHeader.Function);
        Urb->UrbHeader.Status = USBD_STATUS_CANCELED;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Urb->UrbControlTransfer.TransferBufferLength = 0;

        if (Urb->UrbHeader.Function == URB_FUNCTION_ISOCH_TRANSFER)
        {
            for (Index = 0; Index < Urb->UrbIsochronousTransfer.NumberOfPackets; Index++)
                Urb->UrbIsochronousTransfer.IsoPacket[Index].Status = 0xFFFFFFFF;
            Urb->UrbIsochronousTransfer.ErrorCount = 0;
        }

        Device->m_TransferFailureCount++;
    }
    else if (!USBD_SUCCESS(UsbdStatus))
    {
        DPRINT("URB %p function 0x%x on device %p failed, USBD status 0x%lx\n",
               Urb, Urb->UrbHeader.Function, Device, UsbdStatus);
        Device->m_TransferFailureCount++;
        Device->ReportNoPingResponseIfPending();
        Irp->IoStatus.Status = UcxUsbdStatusToNtStatus(UsbdStatus);
    }
    else if (!NT_SUCCESS(Status))
    {
        DPRINT1("URB %p succeeded but its IRP %p failed 0x%lx\n", Urb, Irp, Status);
        NT_ASSERT(FALSE);
        Urb->UrbHeader.Status = UcxNtStatusToUsbdStatus(Status);
    }

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    UcxUnlockTransferBuffer(Urb);

    if (UcxIsXrbIrp(Irp, Urb))
        UcxXrbMarkInactive(Urb);

    return STATUS_CONTINUE_COMPLETION;
}

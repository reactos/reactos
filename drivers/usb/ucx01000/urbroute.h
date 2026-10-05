/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USBPORT style URB processing
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Low 24 bits of URB_HEADER.UsbdFlags belong to UCX, which clears them on entry */
#define UCX_URB_FLAGS_KEEP_HUB_BITS  UCXHUB_URB_FLAGS_HUB_MASK

/* UsbdFlags bit: UCX built and locked TransferBufferMDL and undoes that at completion */
#define UCX_URB_FLAG_BUFFER_LOCKED   0x20000000

/* Control transfers never exceed 64 KB minus one, whatever the pipe allows */
#define UCX_MAX_CONTROL_TRANSFER     0xFFFF

/* First ULONG of the HCD area: the processor index the transfer was submitted on */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)
#endif

/* ReactOS has no KeGetCurrentProcessorNumberEx export yet */
FORCEINLINE
VOID
NTAPI
UcxStampProcessorNumber(
    _Inout_ PURB Urb)
{
    *(PULONG)&Urb->UrbControlTransfer.hca = KeGetCurrentProcessorNumber();
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/** Pins TransferFlags' direction to the pipe; the client's own bit is ignored. */
FORCEINLINE
VOID
NTAPI
UcxSetTransferDirection(
    _Inout_ PURB Urb,
    _In_ const UcxPipe* Pipe)
{
    if (Pipe->DirectionIn)
        Urb->UrbControlTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
    else
        Urb->UrbControlTransfer.TransferFlags &= ~USBD_TRANSFER_DIRECTION_IN;
}

/** Below DISPATCH_LEVEL, locks a buffer passed without an MDL so the HCD can map it. */
VOID
NTAPI
UcxLockTransferBuffer(
    _Inout_ PURB Urb);

VOID
NTAPI
UcxUnlockTransferBuffer(
    _Inout_ PURB Urb);

NTSTATUS
NTAPI
UcxUsbdStatusToNtStatus(
    _In_ USBD_STATUS UsbdStatus);

USBD_STATUS
NTAPI
UcxNtStatusToUsbdStatus(
    _In_ NTSTATUS Status);

/** Validates and routes a URB the way USBPORT did. Owns the IRP afterwards. */
NTSTATUS
NTAPI
UcxProcessLegacyUrb(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb);

BOOLEAN
NTAPI
UcxValidatePipeHandle(
    _In_ UcxUsbDevice* Device,
    _In_ USBD_PIPE_HANDLE Handle);

/* Hands an IRP to a queue on the controller FDO with a completion routine on the PDO's location */

NTSTATUS
NTAPI
UcxForwardIrpToQueue(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE Completion,
    _In_ PVOID Context,
    _In_ WDFQUEUE Queue);

NTSTATUS
NTAPI
UcxForwardIrpWithRequest(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE Completion,
    _In_ PVOID Context,
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request);

#pragma once

#include "hardware.h"
#include "hwiface.h"
#include "physmem.h"
#include "usbehci.h"
#include <usb.h>
#include <ntddk.h>

PQUEUE_HEAD
BuildControlTransfer(PEHCI_HOST_CONTROLLER hcd,
                     ULONG DeviceAddress,
                     USBD_PIPE_HANDLE PipeHandle,
                     PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup,
                     PMDL pMdl,
                     BOOLEAN FreeMdl);

PQUEUE_HEAD
BuildBulkTransfer(PEHCI_HOST_CONTROLLER hcd,
                   ULONG DeviceAddress,
                   USBD_PIPE_HANDLE PipeHandle,
                   UCHAR PidDirection,
                   PMDL pMdl,
                   BOOLEAN FreeMdl);

VOID
BuildSetupPacketFromURB(PEHCI_HOST_CONTROLLER hcd,
                        PURB Urb,
                        PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup);

NTSTATUS
ExecuteTransfer(PDEVICE_OBJECT DeviceObject,
                PUSB_DEVICE UsbDevice,
                USBD_PIPE_HANDLE PipeHandle,
                PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup,
                ULONG TransferFlags,
                PVOID TransferBufferOrMdl,
                ULONG TransferBufferLength,
                PIRP IrpToComplete);

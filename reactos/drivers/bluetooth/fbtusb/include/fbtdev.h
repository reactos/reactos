// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#ifndef _FREEBT_DEV_H
#define _FREEBT_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS FreeBT_DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FreeBT_DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FreeBT_DispatchDevCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FreeBT_ResetPipe(IN PDEVICE_OBJECT	DeviceObject, IN USBD_PIPE_HANDLE PipeInfo);
NTSTATUS FreeBT_ResetDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS FreeBT_GetPortStatus(IN PDEVICE_OBJECT DeviceObject, IN PULONG PortStatus);
NTSTATUS FreeBT_ResetParentPort(IN IN PDEVICE_OBJECT DeviceObject);

NTSTATUS SubmitIdleRequestIrp(IN PDEVICE_EXTENSION DeviceExtension);
VOID IdleNotificationCallback(IN PDEVICE_EXTENSION DeviceExtension);
NTSTATUS IdleNotificationRequestComplete(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension);

VOID CancelSelectSuspend(IN PDEVICE_EXTENSION DeviceExtension);
VOID PoIrpCompletionFunc(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus);

VOID PoIrpAsyncCompletionFunc(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus);

VOID WWIrpCompletionFunc(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus);

#ifdef __cplusplus
};
#endif

#endif

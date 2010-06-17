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

#ifndef _FREEBT_PNP_H
#define _FREEBT_PNP_H

#define REMOTE_WAKEUP_MASK 0x20

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS FreeBT_DispatchPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleStartDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleQueryStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleQueryRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleCancelRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleSurpriseRemoval(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleCancelStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS HandleQueryCapabilities(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS ReadandSelectDescriptors(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS ConfigureDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS SelectInterfaces(IN PDEVICE_OBJECT DeviceObject, IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);
NTSTATUS DeconfigureDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS CallUSBD(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb);
VOID ProcessQueuedRequests(IN OUT PDEVICE_EXTENSION DeviceExtension);
NTSTATUS FreeBT_GetRegistryDword(IN PWCHAR RegPath, IN PWCHAR ValueName, IN OUT PULONG Value);
NTSTATUS FreeBT_DispatchClean(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID DpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID IdleRequestWorkerRoutine(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context);
NTSTATUS FreeBT_AbortPipes(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS IrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS CanStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS CanRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS ReleaseMemory(IN PDEVICE_OBJECT DeviceObject);
LONG FreeBT_IoIncrement(IN OUT PDEVICE_EXTENSION DeviceExtension);
LONG FreeBT_IoDecrement(IN OUT PDEVICE_EXTENSION DeviceExtension);
BOOLEAN CanDeviceSuspend(IN PDEVICE_EXTENSION DeviceExtension);
PCHAR PnPMinorFunctionString (IN UCHAR MinorFunction);

#ifdef __cplusplus
};
#endif

#endif

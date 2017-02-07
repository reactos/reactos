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

NTSTATUS NTAPI FreeBT_DispatchPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleStartDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleQueryStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleQueryRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleCancelRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleSurpriseRemoval(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleCancelStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI HandleQueryCapabilities(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI ReadandSelectDescriptors(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS NTAPI ConfigureDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS NTAPI SelectInterfaces(IN PDEVICE_OBJECT DeviceObject, IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);
NTSTATUS NTAPI DeconfigureDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS NTAPI CallUSBD(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb);
VOID NTAPI ProcessQueuedRequests(IN OUT PDEVICE_EXTENSION DeviceExtension);
NTSTATUS NTAPI FreeBT_GetRegistryDword(IN PWCHAR RegPath, IN PWCHAR ValueName, IN OUT PULONG Value);
NTSTATUS NTAPI FreeBT_DispatchClean(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID NTAPI DpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID NTAPI IdleRequestWorkerRoutine(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context);
NTSTATUS NTAPI FreeBT_AbortPipes(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS NTAPI IrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS NTAPI CanStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI CanRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI ReleaseMemory(IN PDEVICE_OBJECT DeviceObject);
LONG NTAPI FreeBT_IoIncrement(IN OUT PDEVICE_EXTENSION DeviceExtension);
LONG NTAPI FreeBT_IoDecrement(IN OUT PDEVICE_EXTENSION DeviceExtension);
BOOLEAN NTAPI CanDeviceSuspend(IN PDEVICE_EXTENSION DeviceExtension);
PCHAR NTAPI PnPMinorFunctionString (IN UCHAR MinorFunction);

#ifdef __cplusplus
};
#endif

#endif

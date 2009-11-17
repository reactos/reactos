#ifndef KSECDD_H
#define KSECDD_H

#include <ntddk.h>

#define DEVICE_NAME L"\\Device\\KsecDD"

/* dispatch.c */

NTSTATUS NTAPI
KsecDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/* ksecdd.c */

DRIVER_INITIALIZE DriverEntry;

VOID NTAPI
KsecInitializeFunctionPointers(PDRIVER_OBJECT DriverObject);

#endif /* KSECDD_H */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/storage/fdc/fdc/fdc.h
 * PURPOSE:         Floppy class driver header
 *
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <wdm.h>

int _cdecl swprintf(const WCHAR *, ...);

typedef struct _FDC_COMMON_EXTENSION
{
    BOOLEAN IsFDO;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
} FDC_COMMON_EXTENSION, *PFDC_COMMON_EXTENSION;

typedef struct _FDC_FDO_EXTENSION
{
    FDC_COMMON_EXTENSION Common;
    
    LIST_ENTRY FloppyDriveList;
    ULONG FloppyDriveListCount;
    KSPIN_LOCK FloppyDriveListLock;
    
    PDEVICE_OBJECT Ldo;

    CM_FLOPPY_DEVICE_DATA FloppyDeviceData;
} FDC_FDO_EXTENSION, *PFDC_FDO_EXTENSION;

typedef struct _FDC_PDO_EXTENSION
{
    FDC_COMMON_EXTENSION Common;
    
    ULONG FloppyNumber;

    PFDC_FDO_EXTENSION FdoDevExt;
    
    LIST_ENTRY ListEntry;
} FDC_PDO_EXTENSION, *PFDC_PDO_EXTENSION;

/* fdo.c */
NTSTATUS
FdcFdoPnpDispatch(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp);

NTSTATUS
FdcFdoPowerDispatch(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp);

NTSTATUS
FdcFdoDeviceControlDispatch(PDEVICE_OBJECT DeviceObject,
                            PIRP Irp);

NTSTATUS
FdcFdoInternalDeviceControlDispatch(PDEVICE_OBJECT DeviceObject,
                                    PIRP Irp);

/* pdo.c */
NTSTATUS
FdcPdoPnpDispatch(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp);

NTSTATUS
FdcPdoPowerDispatch(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp);

NTSTATUS
FdcPdoDeviceControlDispatch(PDEVICE_OBJECT DeviceObject,
                            PIRP Irp);

NTSTATUS
FdcPdoInternalDeviceControlDispatch(PDEVICE_OBJECT DeviceObject,
                                    PIRP Irp);

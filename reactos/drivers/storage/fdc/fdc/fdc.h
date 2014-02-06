/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/fdc.h
 * PURPOSE:        Common header file
 * PROGRAMMERS:    Eric Kohl
 */

#ifndef _FDC_PCH_
#define _FDC_PCH_

#include <ntifs.h>

#define MAX_DEVICE_NAME 255
#define MAX_ARC_PATH_LEN 255
#define MAX_DRIVES_PER_CONTROLLER 4
#define MAX_CONTROLLERS 4

struct _CONTROLLER_INFO;

typedef struct _DRIVE_INFO
{
    struct _CONTROLLER_INFO  *ControllerInfo;
    UCHAR                    UnitNumber; /* 0,1,2,3 */
    ULONG                    PeripheralNumber;
    PDEVICE_OBJECT           DeviceObject;
    CM_FLOPPY_DEVICE_DATA    FloppyDeviceData;
//    LARGE_INTEGER            MotorStartTime;
//    DISK_GEOMETRY            DiskGeometry;
//    UCHAR                    BytesPerSectorCode;
//    WCHAR                    SymLinkBuffer[MAX_DEVICE_NAME];
//    WCHAR                    ArcPathBuffer[MAX_ARC_PATH_LEN];
//    ULONG                    DiskChangeCount;
//    BOOLEAN                  Initialized;
} DRIVE_INFO, *PDRIVE_INFO;

typedef struct _CONTROLLER_INFO
{
    BOOLEAN          Populated;
//    BOOLEAN          Initialized;
//    ULONG            ControllerNumber;
//    INTERFACE_TYPE   InterfaceType;
//    ULONG            BusNumber;
//    ULONG            Level;
//    KIRQL            MappedLevel;
//    ULONG            Vector;
//    ULONG            MappedVector;
//    KINTERRUPT_MODE  InterruptMode;
    PUCHAR           BaseAddress;
//    ULONG            Dma;
//    ULONG            MapRegisters;
//    PVOID            MapRegisterBase;
//    BOOLEAN          Master;
//    KEVENT           SynchEvent;
//    KDPC             Dpc;
//    PKINTERRUPT      InterruptObject;
//    PADAPTER_OBJECT  AdapterObject;
    UCHAR            NumberOfDrives;
//    BOOLEAN          ImpliedSeeks;
    DRIVE_INFO       DriveInfo[MAX_DRIVES_PER_CONTROLLER];
//    PDRIVE_INFO      CurrentDrive;
//    BOOLEAN          Model30;
//    KEVENT           MotorStoppedEvent;
//    KTIMER           MotorTimer;
//    KDPC             MotorStopDpc;
//    BOOLEAN          StopDpcQueued;
} CONTROLLER_INFO, *PCONTROLLER_INFO;


typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;
    PDEVICE_OBJECT DeviceObject;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;

    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT Pdo;

    CONTROLLER_INFO ControllerInfo;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


typedef struct _PDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;

    PDEVICE_OBJECT Fdo;
    PDRIVE_INFO DriveInfo;

    UNICODE_STRING DeviceDescription; // REG_SZ
    UNICODE_STRING DeviceId;          // REG_SZ
    UNICODE_STRING InstanceId;        // REG_SZ
    UNICODE_STRING HardwareIds;       // REG_MULTI_SZ
    UNICODE_STRING CompatibleIds;     // REG_MULTI_SZ
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

#define FDC_TAG 'acdF'

/* fdo.c */

NTSTATUS
NTAPI
FdcFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* misc.c */

NTSTATUS
DuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString);

/* pdo.c */

NTSTATUS
NTAPI
FdcPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

#endif /* _FDC_PCH_ */

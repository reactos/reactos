/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/fdc.h
 * PURPOSE:        Common header file
 * PROGRAMMERS:    Eric Kohl
 */

#include <ntddk.h>
#include <debug.h>

#define MAX_DEVICE_NAME 255
#define MAX_ARC_PATH_LEN 255
#define MAX_DRIVES_PER_CONTROLLER 4
#define MAX_CONTROLLERS 4

struct _CONTROLLER_INFO;

typedef struct _DRIVE_INFO
{
    struct _CONTROLLER_INFO  *ControllerInfo;
    UCHAR                    UnitNumber; /* 0,1,2,3 */
    LARGE_INTEGER            MotorStartTime;
    PDEVICE_OBJECT           DeviceObject;
    CM_FLOPPY_DEVICE_DATA    FloppyDeviceData;
//    DISK_GEOMETRY            DiskGeometry;
    UCHAR                    BytesPerSectorCode;
    WCHAR                    SymLinkBuffer[MAX_DEVICE_NAME];
    WCHAR                    ArcPathBuffer[MAX_ARC_PATH_LEN];
    ULONG                    DiskChangeCount;
    BOOLEAN                  Initialized;
} DRIVE_INFO, *PDRIVE_INFO;

typedef struct _CONTROLLER_INFO
{
    BOOLEAN          Populated;
    BOOLEAN          Initialized;
    ULONG            ControllerNumber;
    INTERFACE_TYPE   InterfaceType;
    ULONG            BusNumber;
    ULONG            Level;
    KIRQL            MappedLevel;
    ULONG            Vector;
    ULONG            MappedVector;
    KINTERRUPT_MODE  InterruptMode;
    PUCHAR           BaseAddress;
    ULONG            Dma;
    ULONG            MapRegisters;
    PVOID            MapRegisterBase;
    BOOLEAN          Master;
    KEVENT           SynchEvent;
    KDPC             Dpc;
    PKINTERRUPT      InterruptObject;
    PADAPTER_OBJECT  AdapterObject;
    UCHAR            NumberOfDrives;
    BOOLEAN          ImpliedSeeks;
    DRIVE_INFO       DriveInfo[MAX_DRIVES_PER_CONTROLLER];
    PDRIVE_INFO      CurrentDrive;
    BOOLEAN          Model30;
    KEVENT           MotorStoppedEvent;
    KTIMER           MotorTimer;
    KDPC             MotorStopDpc;
    BOOLEAN          StopDpcQueued;
} CONTROLLER_INFO, *PCONTROLLER_INFO;


typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;

    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT Fdo;
    PDEVICE_OBJECT Pdo;

    CONTROLLER_INFO ControllerInfo;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


/* fdo.c */

NTSTATUS
NTAPI
FdcAddDevice(IN PDRIVER_OBJECT DriverObject,
             IN PDEVICE_OBJECT Pdo);

NTSTATUS
NTAPI
FdcFdoPnp(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

/* pdo.c */

NTSTATUS
NTAPI
FdcPdoPnp(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);


/* EOF */
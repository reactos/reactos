/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fs_rec.h

Abstract:

    This module contains the main header file for the mini-file system
    recognizer driver.

Author:

    Darryl E. Havens (darrylh) 22-nov-1993

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "ntifs.h"
#include "ntdddisk.h"
#include "ntddcdrm.h"

//
//  Define the debug trace levels.
//

#define FSREC_DEBUG_LEVEL_FSREC     0x00000001
#define FSREC_DEBUG_LEVEL_NTFS      0x00000002
#define FSREC_DEBUG_LEVEL_CDFS      0x00000004
#define FSREC_DEBUG_LEVEL_UDFS      0x00000008
#define FSREC_DEBUG_LEVEL_FAT       0x00000010

#define FSREC_POOL_TAG		    'crsF' 

#define SetFlag(Flags,SingleFlag) (     \
    (Flags) |= (SingleFlag)             \
)

#define ClearFlag(Flags,SingleFlag) (   \
    (Flags) &= ~(SingleFlag)            \
)

//
// Define the file system types for the device extension.
//

typedef enum _FILE_SYSTEM_TYPE {
    CdfsFileSystem = 1,
    FatFileSystem,
    HpfsFileSystem,
    NtfsFileSystem,
    UdfsFileSystem
} FILE_SYSTEM_TYPE, *PFILE_SYSTEM_TYPE;

//
// Define the device extension for this driver.
//

typedef enum _RECOGNIZER_STATE {
    Active,
    Transparent,
    FastUnload
} RECOGNIZER_STATE, *PRECOGNIZER_STATE;

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT CoRecognizer;
    FILE_SYSTEM_TYPE FileSystemType;
    RECOGNIZER_STATE State;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Define the functions provided by this driver.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
FsRecCleanupClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FsRecCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FsRecCreateAndRegisterDO(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT HeadRecognizer OPTIONAL,
    OUT PDEVICE_OBJECT *NewRecognizer OPTIONAL,
    IN PWCHAR RecFileSystem,
    IN PWCHAR FileSystemName,
    IN FILE_SYSTEM_TYPE FileSystemType,
    IN DEVICE_TYPE DeviceType
    );

NTSTATUS
FsRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FsRecUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
FsRecLoadFileSystem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PWCHAR DriverServiceKey
    );

BOOLEAN
FsRecGetDeviceSectorSize (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG BytesPerSector
    );

BOOLEAN
FsRecGetDeviceSectors (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BytesPerSector,
    OUT PLARGE_INTEGER NumberOfSectors
    );

BOOLEAN
FsRecReadBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLARGE_INTEGER ByteOffset,
    IN ULONG MinimumBytes,
    IN ULONG BytesPerSector,
    OUT PVOID *Buffer,
    OUT PBOOLEAN IsDeviceFailure OPTIONAL
    );

#if DBG

extern LONG FsRecDebugTraceLevel;
extern LONG FsRecDebugTraceIndent;

BOOLEAN
FsRecDebugTrace (
    LONG IndentIncrement,
    ULONG TraceMask,
    PCHAR Format,
    ...
    );

#define DebugTrace(M) FsRecDebugTrace M

#else

#define DebugTrace(M) TRUE

#endif


//
//  Define the per-type recognizers.
//

NTSTATUS
CdfsRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UdfsRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FatRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// Define external functions.
//

NTSTATUS
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

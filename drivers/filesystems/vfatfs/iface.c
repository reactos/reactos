/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entry interface
 * COPYRIGHT:   Copyright 1998 Jason Filby <jasonfilby@yahoo.com>
 *              Copyright 2010-2018 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

PVFAT_GLOBAL_DATA VfatGlobalData;

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\FatX");
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    Status = IoCreateDevice(DriverObject,
                            sizeof(VFAT_GLOBAL_DATA),
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    VfatGlobalData = DeviceObject->DeviceExtension;
    RtlZeroMemory (VfatGlobalData, sizeof(VFAT_GLOBAL_DATA));
    VfatGlobalData->DriverObject = DriverObject;
    VfatGlobalData->DeviceObject = DeviceObject;
    VfatGlobalData->NumberProcessors = KeNumberProcessors;
    /* Enable this to enter the debugger when file system corruption
     * has been detected:
    VfatGlobalData->Flags = VFAT_BREAK_ON_CORRUPTION; */

    /* Delayed close support */
    ExInitializeFastMutex(&VfatGlobalData->CloseMutex);
    InitializeListHead(&VfatGlobalData->CloseListHead);
    VfatGlobalData->CloseCount = 0;
    VfatGlobalData->CloseWorkerRunning = FALSE;
    VfatGlobalData->ShutdownStarted = FALSE;
    VfatGlobalData->CloseWorkItem = IoAllocateWorkItem(DeviceObject);
    if (VfatGlobalData->CloseWorkItem == NULL)
    {
        IoDeleteDevice(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeviceObject->Flags |= DO_DIRECT_IO;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_READ] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = VfatShutdown;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_PNP] = VfatBuildRequest;

    DriverObject->DriverUnload = NULL;

    /* Cache manager */
    VfatGlobalData->CacheMgrCallbacks.AcquireForLazyWrite = VfatAcquireForLazyWrite;
    VfatGlobalData->CacheMgrCallbacks.ReleaseFromLazyWrite = VfatReleaseFromLazyWrite;
    VfatGlobalData->CacheMgrCallbacks.AcquireForReadAhead = VfatAcquireForLazyWrite;
    VfatGlobalData->CacheMgrCallbacks.ReleaseFromReadAhead = VfatReleaseFromLazyWrite;

    /* Fast I/O */
    VfatInitFastIoRoutines(&VfatGlobalData->FastIoDispatch);
    DriverObject->FastIoDispatch = &VfatGlobalData->FastIoDispatch;

    /* Private lists */
    ExInitializeNPagedLookasideList(&VfatGlobalData->FcbLookasideList,
                                    NULL, NULL, 0, sizeof(VFATFCB), TAG_FCB, 0);
    ExInitializeNPagedLookasideList(&VfatGlobalData->CcbLookasideList,
                                    NULL, NULL, 0, sizeof(VFATCCB), TAG_CCB, 0);
    ExInitializeNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList,
                                    NULL, NULL, 0, sizeof(VFAT_IRP_CONTEXT), TAG_IRP, 0);
    ExInitializePagedLookasideList(&VfatGlobalData->CloseContextLookasideList,
                                   NULL, NULL, 0, sizeof(VFAT_CLOSE_CONTEXT), TAG_CLOSE, 0);

    ExInitializeResourceLite(&VfatGlobalData->VolumeListLock);
    InitializeListHead(&VfatGlobalData->VolumeListHead);
    IoRegisterFileSystem(DeviceObject);

#ifdef KDBG
    {
        BOOLEAN Registered;

        Registered = KdRosRegisterCliCallback(vfatKdbgHandler);
        DPRINT1("VFATFS KDBG extension registered: %s\n", (Registered ? "yes" : "no"));
    }
#endif

    return STATUS_SUCCESS;
}

/* EOF */

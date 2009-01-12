/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fastfat.c
 * PURPOSE:         Initialization routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* GLOBALS ******************************************************************/

FAT_GLOBAL_DATA FatGlobalData;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Fat");
    NTSTATUS Status;

    /* Create a device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status)) return Status;

    /* Zero global storage */
    RtlZeroMemory(&FatGlobalData, sizeof(FAT_GLOBAL_DATA));
    FatGlobalData.DriverObject = DriverObject;
    FatGlobalData.DiskDeviceObject = DeviceObject;

    /* Fill major function handlers */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FatClose;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FatCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = FatRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = FatWrite;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FatFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = FatQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = FatSetInformation;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = FatDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = FatQueryVolumeInfo;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = FatSetVolumeInfo;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = FatShutdown;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = FatLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FatDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FatCleanup;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = FatFlushBuffers;
    //DriverObject->MajorFunction[IRP_MJ_QUERY_EA]
    //DriverObject->MajorFunction[IRP_MJ_SET_EA]
    //DriverObject->MajorFunction[IRP_MJ_PNP]

    DriverObject->DriverUnload = NULL;

    /* Initialize cache manager callbacks */
    FatGlobalData.CacheMgrCallbacks.AcquireForLazyWrite = FatAcquireForLazyWrite;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromLazyWrite = FatReleaseFromLazyWrite;
    FatGlobalData.CacheMgrCallbacks.AcquireForReadAhead = FatAcquireForReadAhead;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromReadAhead = FatReleaseFromReadAhead;

    FatGlobalData.CacheMgrCallbacks.AcquireForLazyWrite = FatNoopAcquire;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromLazyWrite = FatNoopRelease;
    FatGlobalData.CacheMgrCallbacks.AcquireForReadAhead = FatNoopAcquire;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromReadAhead = FatNoopRelease;

    /* Initialize Fast I/O dispatchers */
    FatInitFastIoRoutines(&FatGlobalData.FastIoDispatch);
    DriverObject->FastIoDispatch = &FatGlobalData.FastIoDispatch;

    /* Initialize lookaside lists */
    ExInitializeNPagedLookasideList(&FatGlobalData.NonPagedFcbList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(VFATFCB),
                                    TAG_FCB,
                                    0);

    ExInitializeNPagedLookasideList(&FatGlobalData.ResourceList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(ERESOURCE),
                                    TAG_CCB,
                                    0);

    ExInitializeNPagedLookasideList(&FatGlobalData.IrpContextList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FAT_IRP_CONTEXT),
                                    TAG_IRP,
                                    0);

    /* Initialize synchronization resource for the global data */
    ExInitializeResourceLite(&FatGlobalData.Resource);

    /* Register and reference our filesystem */
    IoRegisterFileSystem(DeviceObject);
    ObReferenceObject(DeviceObject);

    return STATUS_SUCCESS;
}

/* EOF */


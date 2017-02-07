/*
* COPYRIGHT:  See COPYING in the top level directory
* PROJECT:    ReactOS kernel
* FILE:       drivers/fs/np/mount.c
* PURPOSE:    Named pipe filesystem
* PROGRAMMER: David Welch <welch@cwcom.net>
*/

/* INCLUDES ******************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    PNPFS_VCB Vcb;
    PNPFS_FCB Fcb;
    NTSTATUS Status;

    DPRINT("Named Pipe FSD 0.0.2\n");

    ASSERT (sizeof(NPFS_CONTEXT) <= FIELD_OFFSET(IRP, Tail.Overlay.DriverContext));
    ASSERT (sizeof(NPFS_WAITER_ENTRY) <= FIELD_OFFSET(IRP, Tail.Overlay.DriverContext));

    DriverObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
        NpfsCreateNamedPipe;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NpfsClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = NpfsRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NpfsWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
        NpfsQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
        NpfsSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
        NpfsQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpfsFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
        NpfsDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
        NpfsFileSystemControl;
    //   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] =
    //     NpfsQuerySecurity;
    //   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
    //     NpfsSetSecurity;

    DriverObject->DriverUnload = NULL;

    RtlInitUnicodeString(&DeviceName, L"\\Device\\NamedPipe");
    Status = IoCreateDevice(DriverObject,
        sizeof(NPFS_VCB),
        &DeviceName,
        FILE_DEVICE_NAMED_PIPE,
        0,
        FALSE,
        &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create named pipe device! (Status %x)\n", Status);
        return Status;
    }

    /* Initialize the device object */
    DeviceObject->Flags |= DO_DIRECT_IO;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Initialize the Volume Control Block (VCB) */
    Vcb = (PNPFS_VCB)DeviceObject->DeviceExtension;
    InitializeListHead(&Vcb->PipeListHead);
    InitializeListHead(&Vcb->ThreadListHead);
    KeInitializeMutex(&Vcb->PipeListLock, 0);
    Vcb->EmptyWaiterCount = 0;

    /* set the size quotas */
    Vcb->MinQuota = PAGE_SIZE;
    Vcb->DefaultQuota = 8 * PAGE_SIZE;
    Vcb->MaxQuota = 64 * PAGE_SIZE;

    /* Create the device FCB */
    Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(NPFS_FCB), TAG_NPFS_FCB);
    Fcb->Type = FCB_DEVICE;
    Fcb->Vcb = Vcb;
    Fcb->RefCount = 1;
    Vcb->DeviceFcb = Fcb;

    /* Create the root directory FCB */
    Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(NPFS_FCB), TAG_NPFS_FCB);
    Fcb->Type = FCB_DIRECTORY;
    Fcb->Vcb = Vcb;
    Fcb->RefCount = 1;
    Vcb->RootFcb = Fcb;

    return STATUS_SUCCESS;
}


FCB_TYPE
NpfsGetFcb(PFILE_OBJECT FileObject,
           PNPFS_FCB *Fcb)
{
    PNPFS_FCB LocalFcb = NULL;
    FCB_TYPE FcbType = FCB_INVALID;

    _SEH2_TRY
    {
        LocalFcb = (PNPFS_FCB)FileObject->FsContext;
        FcbType = LocalFcb->Type;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        LocalFcb = NULL;
        FcbType = FCB_INVALID;
    }
    _SEH2_END;

    *Fcb = LocalFcb;

    return FcbType;
}


CCB_TYPE
NpfsGetCcb(PFILE_OBJECT FileObject,
           PNPFS_CCB *Ccb)
{
    PNPFS_CCB LocalCcb = NULL;
    CCB_TYPE CcbType = CCB_INVALID;

    _SEH2_TRY
    {
        LocalCcb = (PNPFS_CCB)FileObject->FsContext2;
        CcbType = LocalCcb->Type;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        LocalCcb = NULL;
        CcbType = CCB_INVALID;
    }
    _SEH2_END;

    *Ccb = LocalCcb;

    return CcbType;
}

/* EOF */

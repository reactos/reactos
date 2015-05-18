/*
 * FILE:             drivers/filesystems/fastfat/fastio.c
 * PURPOSE:          Fast IO routines.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Herve Poussineau (hpoussin@reactos.org)
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

#include "vfat.h"

#define NDEBUG
#include <debug.h>

static FAST_IO_CHECK_IF_POSSIBLE VfatFastIoCheckIfPossible;

static
BOOLEAN
NTAPI
VfatFastIoCheckIfPossible(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    /* Prevent all Fast I/O requests */
    DPRINT("VfatFastIoCheckIfPossible(): returning FALSE.\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(CheckForReadOperation);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_READ VfatFastIoRead;

static
BOOLEAN
NTAPI
VfatFastIoRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoRead()\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_WRITE VfatFastIoWrite;

static
BOOLEAN
NTAPI
VfatFastIoWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoWrite()\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_QUERY_BASIC_INFO VfatFastIoQueryBasicInfo;

static
BOOLEAN
NTAPI
VfatFastIoQueryBasicInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PVFATFCB FCB = NULL;
    BOOLEAN Success = FALSE;
    ULONG BufferLength = sizeof(FILE_BASIC_INFORMATION);

    DPRINT("VfatFastIoQueryBasicInfo()\n");

    FCB = (PVFATFCB)FileObject->FsContext;
    if (FCB == NULL)
    {
        return FALSE;
    }

    FsRtlEnterFileSystem();

    if (!(FCB->Flags & FCB_IS_PAGE_FILE))
    {
        if (!ExAcquireResourceSharedLite(&FCB->MainResource, Wait))
        {
            FsRtlExitFileSystem();
            return FALSE;
        }
    }

    Status = VfatGetBasicInformation(FileObject,
                                     FCB,
                                     DeviceObject,
                                     Buffer,
                                     &BufferLength);

    if (!(FCB->Flags & FCB_IS_PAGE_FILE))
    {
        ExReleaseResourceLite(&FCB->MainResource);
    }

    if (NT_SUCCESS(Status))
    {
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_BASIC_INFORMATION) - BufferLength;
        Success = TRUE;
    }

    FsRtlExitFileSystem();

    return Success;
}

static FAST_IO_QUERY_STANDARD_INFO VfatFastIoQueryStandardInfo;

static
BOOLEAN
NTAPI
VfatFastIoQueryStandardInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PVFATFCB FCB = NULL;
    BOOLEAN Success = FALSE;
    ULONG BufferLength = sizeof(FILE_STANDARD_INFORMATION);

    DPRINT("VfatFastIoQueryStandardInfo()\n");

    UNREFERENCED_PARAMETER(DeviceObject);

    FCB = (PVFATFCB)FileObject->FsContext;
    if (FCB == NULL)
    {
        return FALSE;
    }

    FsRtlEnterFileSystem();

    if (!(FCB->Flags & FCB_IS_PAGE_FILE))
    {
        if (!ExAcquireResourceSharedLite(&FCB->MainResource, Wait))
        {
            FsRtlExitFileSystem();
            return FALSE;
        }
    }

    Status = VfatGetStandardInformation(FCB,
                                        Buffer,
                                        &BufferLength);

    if (!(FCB->Flags & FCB_IS_PAGE_FILE))
    {
        ExReleaseResourceLite(&FCB->MainResource);
    }

    if (NT_SUCCESS(Status))
    {
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION) - BufferLength;
        Success = TRUE;
    }

    FsRtlExitFileSystem();

    return Success;
}

static FAST_IO_LOCK VfatFastIoLock;

static
BOOLEAN
NTAPI
VfatFastIoLock(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoLock\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(FailImmediately);
    UNREFERENCED_PARAMETER(ExclusiveLock);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_UNLOCK_SINGLE VfatFastIoUnlockSingle;

static
BOOLEAN
NTAPI
VfatFastIoUnlockSingle(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoUnlockSingle\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_UNLOCK_ALL VfatFastIoUnlockAll;

static
BOOLEAN
NTAPI
VfatFastIoUnlockAll(
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoUnlockAll\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_UNLOCK_ALL_BY_KEY VfatFastIoUnlockAllByKey;

static
BOOLEAN
NTAPI
VfatFastIoUnlockAllByKey(
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoUnlockAllByKey\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_DEVICE_CONTROL VfatFastIoDeviceControl;

static
BOOLEAN
NTAPI
VfatFastIoDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoDeviceControl\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_ACQUIRE_FILE VfatAcquireFileForNtCreateSection;

static
VOID
NTAPI
VfatAcquireFileForNtCreateSection(
    IN PFILE_OBJECT FileObject)
{
    DPRINT("VfatAcquireFileForNtCreateSection\n");
    UNREFERENCED_PARAMETER(FileObject);
}

static FAST_IO_RELEASE_FILE VfatReleaseFileForNtCreateSection;

static
VOID
NTAPI
VfatReleaseFileForNtCreateSection(
    IN PFILE_OBJECT FileObject)
{
    DPRINT("VfatReleaseFileForNtCreateSection\n");
    UNREFERENCED_PARAMETER(FileObject);
}

static FAST_IO_DETACH_DEVICE VfatFastIoDetachDevice;

static
VOID
NTAPI
VfatFastIoDetachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice)
{
    DPRINT("VfatFastIoDetachDevice\n");
    UNREFERENCED_PARAMETER(SourceDevice);
    UNREFERENCED_PARAMETER(TargetDevice);
}

static FAST_IO_QUERY_NETWORK_OPEN_INFO VfatFastIoQueryNetworkOpenInfo;

static
BOOLEAN
NTAPI
VfatFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoQueryNetworkOpenInfo\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_ACQUIRE_FOR_MOD_WRITE VfatAcquireForModWrite;

static
NTSTATUS
NTAPI
VfatAcquireForModWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE* ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatAcquireForModWrite\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(EndingOffset);
    UNREFERENCED_PARAMETER(ResourceToRelease);
    UNREFERENCED_PARAMETER(DeviceObject);

    return STATUS_INVALID_DEVICE_REQUEST;
}

static FAST_IO_MDL_READ VfatMdlRead;

static
BOOLEAN
NTAPI
VfatMdlRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL* MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatMdlRead\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_MDL_READ_COMPLETE VfatMdlReadComplete;

static
BOOLEAN
NTAPI
VfatMdlReadComplete(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatMdlReadComplete\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_PREPARE_MDL_WRITE VfatPrepareMdlWrite;

static
BOOLEAN
NTAPI
VfatPrepareMdlWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL* MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatPrepareMdlWrite\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_MDL_WRITE_COMPLETE VfatMdlWriteComplete;

static
BOOLEAN
NTAPI
VfatMdlWriteComplete(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatMdlWriteComplete\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_READ_COMPRESSED VfatFastIoReadCompressed;

static
BOOLEAN
NTAPI
VfatFastIoReadCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL* MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoReadCompressed\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(CompressedDataInfo);
    UNREFERENCED_PARAMETER(CompressedDataInfoLength);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_WRITE_COMPRESSED VfatFastIoWriteCompressed;

static
BOOLEAN
NTAPI
VfatFastIoWriteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL* MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoWriteCompressed\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(CompressedDataInfo);
    UNREFERENCED_PARAMETER(CompressedDataInfoLength);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_MDL_READ_COMPLETE_COMPRESSED VfatMdlReadCompleteCompressed;

static
BOOLEAN
NTAPI
VfatMdlReadCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatMdlReadCompleteCompressed\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_MDL_WRITE_COMPLETE_COMPRESSED VfatMdlWriteCompleteCompressed;

static
BOOLEAN
NTAPI
VfatMdlWriteCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatMdlWriteCompleteCompressed\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_QUERY_OPEN VfatFastIoQueryOpen;

static
BOOLEAN
NTAPI
VfatFastIoQueryOpen(
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatFastIoQueryOpen\n");

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(NetworkInformation);
    UNREFERENCED_PARAMETER(DeviceObject);

    return FALSE;
}

static FAST_IO_RELEASE_FOR_MOD_WRITE VfatReleaseForModWrite;

static
NTSTATUS
NTAPI
VfatReleaseForModWrite(
    IN PFILE_OBJECT FileObject,
    IN PERESOURCE ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("VfatReleaseForModWrite\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(ResourceToRelease);
    UNREFERENCED_PARAMETER(DeviceObject);

    return STATUS_INVALID_DEVICE_REQUEST;
}

static FAST_IO_ACQUIRE_FOR_CCFLUSH VfatAcquireForCcFlush;

static
NTSTATUS
NTAPI
VfatAcquireForCcFlush(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject)
{
    PVFATFCB Fcb = (PVFATFCB)FileObject->FsContext;

    DPRINT("VfatAcquireForCcFlush\n");

    UNREFERENCED_PARAMETER(DeviceObject);

    /* Make sure it is not a volume lock */
    ASSERT(!(Fcb->Flags & FCB_IS_VOLUME));

    /* Acquire the resource */
    ExAcquireResourceExclusiveLite(&(Fcb->MainResource), TRUE);

    return STATUS_SUCCESS;
}

static FAST_IO_RELEASE_FOR_CCFLUSH VfatReleaseForCcFlush;

static
NTSTATUS
NTAPI
VfatReleaseForCcFlush(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject)
{
    PVFATFCB Fcb = (PVFATFCB)FileObject->FsContext;

    DPRINT("VfatReleaseForCcFlush\n");

    UNREFERENCED_PARAMETER(DeviceObject);

    /* Make sure it is not a volume lock */
    ASSERT(!(Fcb->Flags & FCB_IS_VOLUME));

    /* Release the resource */
    ExReleaseResourceLite(&(Fcb->MainResource));

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
VfatAcquireForLazyWrite(
    IN PVOID Context,
    IN BOOLEAN Wait)
{
    PVFATFCB Fcb = (PVFATFCB)Context;
    ASSERT(Fcb);
    DPRINT("VfatAcquireForLazyWrite(): Fcb %p\n", Fcb);

    if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
    {
        DPRINT("VfatAcquireForLazyWrite(): ExReleaseResourceLite failed.\n");
        return FALSE;
    }
    return TRUE;
}

VOID
NTAPI
VfatReleaseFromLazyWrite(
    IN PVOID Context)
{
    PVFATFCB Fcb = (PVFATFCB)Context;
    ASSERT(Fcb);
    DPRINT("VfatReleaseFromLazyWrite(): Fcb %p\n", Fcb);

    ExReleaseResourceLite(&(Fcb->MainResource));
}

BOOLEAN
NTAPI
VfatAcquireForReadAhead(
    IN PVOID Context,
    IN BOOLEAN Wait)
{
    PVFATFCB Fcb = (PVFATFCB)Context;
    ASSERT(Fcb);
    DPRINT("VfatAcquireForReadAhead(): Fcb %p\n", Fcb);

    if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
    {
        DPRINT("VfatAcquireForReadAhead(): ExReleaseResourceLite failed.\n");
        return FALSE;
    }
    return TRUE;
}

VOID
NTAPI
VfatReleaseFromReadAhead(
    IN PVOID Context)
{
    PVFATFCB Fcb = (PVFATFCB)Context;
    ASSERT(Fcb);
    DPRINT("VfatReleaseFromReadAhead(): Fcb %p\n", Fcb);

    ExReleaseResourceLite(&(Fcb->MainResource));
}

VOID
VfatInitFastIoRoutines(
    PFAST_IO_DISPATCH FastIoDispatch)
{
    FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch->FastIoCheckIfPossible = VfatFastIoCheckIfPossible;
    FastIoDispatch->FastIoRead = VfatFastIoRead;
    FastIoDispatch->FastIoWrite = VfatFastIoWrite;
    FastIoDispatch->FastIoQueryBasicInfo = VfatFastIoQueryBasicInfo;
    FastIoDispatch->FastIoQueryStandardInfo = VfatFastIoQueryStandardInfo;
    FastIoDispatch->FastIoLock = VfatFastIoLock;
    FastIoDispatch->FastIoUnlockSingle = VfatFastIoUnlockSingle;
    FastIoDispatch->FastIoUnlockAll = VfatFastIoUnlockAll;
    FastIoDispatch->FastIoUnlockAllByKey = VfatFastIoUnlockAllByKey;
    FastIoDispatch->FastIoDeviceControl = VfatFastIoDeviceControl;
    FastIoDispatch->AcquireFileForNtCreateSection = VfatAcquireFileForNtCreateSection;
    FastIoDispatch->ReleaseFileForNtCreateSection = VfatReleaseFileForNtCreateSection;
    FastIoDispatch->FastIoDetachDevice = VfatFastIoDetachDevice;
    FastIoDispatch->FastIoQueryNetworkOpenInfo = VfatFastIoQueryNetworkOpenInfo;
    FastIoDispatch->MdlRead = VfatMdlRead;
    FastIoDispatch->MdlReadComplete = VfatMdlReadComplete;
    FastIoDispatch->PrepareMdlWrite = VfatPrepareMdlWrite;
    FastIoDispatch->MdlWriteComplete = VfatMdlWriteComplete;
    FastIoDispatch->FastIoReadCompressed = VfatFastIoReadCompressed;
    FastIoDispatch->FastIoWriteCompressed = VfatFastIoWriteCompressed;
    FastIoDispatch->MdlReadCompleteCompressed = VfatMdlReadCompleteCompressed;
    FastIoDispatch->MdlWriteCompleteCompressed = VfatMdlWriteCompleteCompressed;
    FastIoDispatch->FastIoQueryOpen = VfatFastIoQueryOpen;
    FastIoDispatch->AcquireForModWrite = VfatAcquireForModWrite;
    FastIoDispatch->ReleaseForModWrite = VfatReleaseForModWrite;
    FastIoDispatch->AcquireForCcFlush = VfatAcquireForCcFlush;
    FastIoDispatch->ReleaseForCcFlush = VfatReleaseForCcFlush;
}

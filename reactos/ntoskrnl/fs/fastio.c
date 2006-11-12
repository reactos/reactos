/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fs/fastio.c
 * PURPOSE:         File System Routines which support Fast I/O or Cc Access.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern ULONG CcFastReadNotPossible;
extern ULONG CcFastReadResourceMiss;
extern ULONG CcFastReadWait;
extern ULONG CcFastReadNoWait;
ULONG CcFastMdlReadNotPossible;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadResourceMiss(VOID)
{
    CcFastReadResourceMiss++;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadNotPossible(VOID)
{
    CcFastReadNotPossible++;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadWait(VOID)
{
    CcFastReadWait++;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadNoWait(VOID)
{
    CcFastReadNoWait++;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlCopyRead(IN PFILE_OBJECT FileObject,
              IN PLARGE_INTEGER FileOffset,
              IN ULONG Length,
              IN BOOLEAN Wait,
              IN ULONG LockKey,
              OUT PVOID Buffer,
              OUT PIO_STATUS_BLOCK IoStatus,
              IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlCopyWrite(IN PFILE_OBJECT FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               IN BOOLEAN Wait,
               IN ULONG LockKey,
               OUT PVOID Buffer,
               OUT PIO_STATUS_BLOCK IoStatus,
               IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlGetFileSize(IN PFILE_OBJECT  FileObject,
                 IN OUT PLARGE_INTEGER FileSize)
{
    FILE_STANDARD_INFORMATION Info;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG Length;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->FastIoQueryStandardInfo)
    {
        /* Fast Path */
        FastDispatch->FastIoQueryStandardInfo(FileObject,
                                              TRUE,
                                              &Info,
                                              &IoStatusBlock,
                                              DeviceObject);
        Status = IoStatusBlock.Status;
    }
    else
    {
        /* Slow Path */
        Status = IoQueryFileInformation(FileObject,
                                        FileStandardInformation,
                                        sizeof(Info),
                                        &Info,
                                        &Length);
    }

    /* Check success */
    if (NT_SUCCESS(Status))
    {
        *FileSize = Info.EndOfFile;
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlRead(IN PFILE_OBJECT FileObject,
             IN PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             IN ULONG LockKey,
             OUT PMDL *MdlChain,
             OUT PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlRead)
    {
        /* Use the fast path */
        return FastDispatch->MdlRead(FileObject,
                                     FileOffset,
                                     Length,
                                     LockKey,
                                     MdlChain,
                                     IoStatus,
                                     DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlRead &&
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlMdlReadDev(FileObject,
                           FileOffset,
                           Length,
                           LockKey,
                           MdlChain,
                           IoStatus,
                           DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadComplete(IN PFILE_OBJECT FileObject,
                     IN OUT PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlReadComplete)
    {
        /* Use the fast path */
        return FastDispatch->MdlReadComplete(FileObject,
                                             MdlChain,
                                             DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlReadComplete &&
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlMdlReadCompleteDev(FileObject, MdlChain, DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadCompleteDev(IN PFILE_OBJECT FileObject,
                        IN PMDL MdlChain,
                        IN PDEVICE_OBJECT DeviceObject)
{
    /* Call the Cache Manager */
    CcMdlReadCompleteDev(MdlChain, FileObject);
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadDev(IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER FileOffset,
                IN ULONG Length,
                IN ULONG LockKey,
                OUT PMDL *MdlChain,
                OUT PIO_STATUS_BLOCK IoStatus,
                IN PDEVICE_OBJECT DeviceObject)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    BOOLEAN Result = TRUE;
    LARGE_INTEGER Offset;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT Device;
    PAGED_CODE();

    /* No actual read */
    if (!Length)
    {
        /* Return success */
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;
        return TRUE;
    }

    /* Sanity check */
    ASSERT(MAXLONGLONG - FileOffset->QuadPart >= (LONGLONG)Length);

    /* Get the offset and FCB header */
    Offset.QuadPart = FileOffset->QuadPart + Length;
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    /* Enter the FS */
    FsRtlEnterFileSystem();
    CcFastMdlReadWait++;

    /* Lock the FCB */
    ExAcquireResourceShared(FcbHeader->Resource, TRUE);

    /* Check if this is a fast I/O cached file */
    if (!(FileObject->PrivateCacheMap) ||
        (FcbHeader->IsFastIoPossible == FastIoIsNotPossible))
    {
        /* It's not, so fail */
        CcFastMdlReadNotPossible += 1;
        Result = FALSE;
        goto Cleanup;
    }

    /* Check if we need to find out if fast I/O is available */
    if (FcbHeader->IsFastIoPossible == FastIoIsQuestionable)
    {
        /* Sanity check */
        ASSERT(!KeIsExecutingDpc());

        /* Get the Fast I/O table */
        Device = IoGetRelatedDeviceObject(FileObject);
        FastIoDispatch = Device->DriverObject->FastIoDispatch;

        /* Sanity check */
        ASSERT(FastIoDispatch != NULL);
        ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

        /* Ask the driver if we can do it */
        if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                   FileOffset,
                                                   Length,
                                                   TRUE,
                                                   LockKey,
                                                   TRUE,
                                                   IoStatus,
                                                   Device))
        {
            /* It's not, fail */
            CcFastMdlReadNotPossible += 1;
            Result = FALSE;
            goto Cleanup;
        }
    }

    /* Check if we read too much */
    if (Offset.QuadPart > FcbHeader->FileSize.QuadPart)
    {
        /* We did, check if the file offset is past the end */
        if (FileOffset->QuadPart >= FcbHeader->FileSize.QuadPart)
        {
            /* Set end of file */
            IoStatus->Status = STATUS_END_OF_FILE;
            IoStatus->Information = 0;
            goto Cleanup;
        }

        /* Otherwise, just normalize the length */
        Length = (ULONG)(FcbHeader->FileSize.QuadPart - FileOffset->QuadPart);
    }

    /* Set this as top-level IRP */
    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    /* Attempt a read */
    CcMdlRead(FileObject, FileOffset, Length, MdlChain, IoStatus);
    FileObject->Flags |= FO_FILE_FAST_IO_READ;

    /* Remove the top-level IRP flag */
    PsGetCurrentThread()->TopLevelIrp = 0;

    /* Return to caller */
Cleanup:
    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();
    return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlWriteComplete(IN PFILE_OBJECT FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlWriteComplete)
    {
        /* Use the fast path */
        return FastDispatch->MdlWriteComplete(FileObject,
                                              FileOffset,
                                              MdlChain,
                                              DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlWriteComplete &&
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlMdlWriteCompleteDev(FileObject,
                                    FileOffset,
                                    MdlChain,
                                    DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlWriteCompleteDev(IN PFILE_OBJECT FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN PMDL MdlChain,
                         IN PDEVICE_OBJECT DeviceObject)
{
    /* Call the Cache Manager */
    CcMdlWriteCompleteDev(FileOffset, MdlChain, FileObject);
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlPrepareMdlWrite(IN PFILE_OBJECT FileObject,
                     IN PLARGE_INTEGER FileOffset,
                     IN ULONG Length,
                     IN ULONG LockKey,
                     OUT PMDL *MdlChain,
                     OUT PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->PrepareMdlWrite)
    {
        /* Use the fast path */
        return FastDispatch->PrepareMdlWrite(FileObject,
                                             FileOffset,
                                             Length,
                                             LockKey,
                                             MdlChain,
                                             IoStatus,
                                             DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->PrepareMdlWrite &&
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlPrepareMdlWriteDev(FileObject,
                                   FileOffset,
                                   Length,
                                   LockKey,
                                   MdlChain,
                                   IoStatus,
                                   DeviceObject);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlPrepareMdlWriteDev(IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN ULONG Length,
                        IN ULONG LockKey,
                        OUT PMDL *MdlChain,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
* @implemented
*/
VOID
NTAPI
FsRtlAcquireFileExclusive(IN PFILE_OBJECT FileObject)
{
    PFAST_IO_DISPATCH FastDispatch;
    PDEVICE_OBJECT DeviceObject;
    PFSRTL_COMMON_FCB_HEADER FcbHeader;

    /* Get the Device Object and fast dispatch */
    DeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we have to do a Fast I/O Dispatch */
    if ((FastDispatch) && (FastDispatch->AcquireFileForNtCreateSection))
    {
        /* Enter the file system and call it */
        FsRtlEnterFileSystem();
        FastDispatch->AcquireFileForNtCreateSection(FileObject);
        return;
    }

    /* Get the FCB Header */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    if (FcbHeader->Resource)
    {
        /* Use a Resource Acquire */
        FsRtlEnterFileSystem();
        ExAcquireResourceExclusive(FcbHeader->Resource, TRUE);
    }

    /* All done, return */
    return;
}

/*
* @implemented
*/
VOID
NTAPI
FsRtlReleaseFile(IN PFILE_OBJECT FileObject)
{
    PFAST_IO_DISPATCH FastDispatch;
    PDEVICE_OBJECT DeviceObject;
    PFSRTL_COMMON_FCB_HEADER FcbHeader;

    /* Get the Device Object */
    DeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);

    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we have to do a Fast I/O Dispatch */
    if ((FastDispatch) && (FastDispatch->ReleaseFileForNtCreateSection))
    {
        /* Call the release function and exit the file system */
        FastDispatch->ReleaseFileForNtCreateSection(FileObject);
        FsRtlExitFileSystem();
        return;
    }

    /* Get the FCB Header */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    if (FcbHeader->Resource)
    {
        /* Use a Resource release */
        ExReleaseResourceLite(FcbHeader->Resource);
        FsRtlExitFileSystem();
    }

    /* All done, return */
    return;
}

/* EOF */

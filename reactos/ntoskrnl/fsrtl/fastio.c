/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/fastio.c
 * PURPOSE:         Provides Fast I/O entrypoints to the Cache Manager
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

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
    KEBUGCHECK(0);
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
    KEBUGCHECK(0);
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
    KEBUGCHECK(0);
    return STATUS_UNSUCCESSFUL;
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
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadComplete(IN PFILE_OBJECT FileObject,
                     IN OUT PMDL MdlChain)
{
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    return FALSE;
}

/*
* @implemented
*/
VOID
NTAPI
FsRtlAcquireFileExclusive(IN PFILE_OBJECT FileObject)
{
    KEBUGCHECK(0);
}

/*
* @implemented
*/
VOID
NTAPI
FsRtlReleaseFile(IN PFILE_OBJECT FileObject)
{
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlRegisterFileSystemFilterCallbacks
 * @unimplemented
 *
 * FILLME
 *
 * @param FilterDriverObject
 *        FILLME
 *
 * @param Callbacks
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlRegisterFileSystemFilterCallbacks(IN PDRIVER_OBJECT FilterDriverObject,
                                       IN PFS_FILTER_CALLBACKS Callbacks)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


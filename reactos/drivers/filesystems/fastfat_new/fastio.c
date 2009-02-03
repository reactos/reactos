/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fastio.c
 * PURPOSE:         Fast IO routines
 * PROGRAMMERS:     Herve Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FatFastIoCheckIfPossible(IN PFILE_OBJECT FileObject,
                          IN PLARGE_INTEGER FileOffset,
                          IN ULONG Lenght,
                          IN BOOLEAN Wait,
                          IN ULONG LockKey,
                          IN BOOLEAN CheckForReadOperation,
                          OUT PIO_STATUS_BLOCK IoStatus,
                          IN PDEVICE_OBJECT DeviceObject)
{
    /* Prevent all Fast I/O requests */
    DPRINT("FatFastIoCheckIfPossible(): returning FALSE.\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoRead(IN PFILE_OBJECT FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG	Length,
               IN BOOLEAN Wait,
               IN ULONG LockKey,
               OUT PVOID Buffer,
               OUT PIO_STATUS_BLOCK	IoStatus,
               IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoRead()\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoWrite(IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER FileOffset,
                IN ULONG Length,
                IN BOOLEAN Wait,
                IN ULONG LockKey,
                OUT PVOID Buffer,
                OUT PIO_STATUS_BLOCK IoStatus,
                IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoWrite()\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoQueryBasicInfo(IN PFILE_OBJECT FileObject,
                         IN BOOLEAN	Wait,
                         OUT PFILE_BASIC_INFORMATION Buffer,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoQueryBasicInfo()\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoQueryStandardInfo(IN PFILE_OBJECT FileObject,
                            IN BOOLEAN Wait,
                            OUT PFILE_STANDARD_INFORMATION Buffer,
                            OUT PIO_STATUS_BLOCK IoStatus,
                            IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoQueryStandardInfo\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoLock(IN PFILE_OBJECT FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN PLARGE_INTEGER Length,
               PEPROCESS ProcessId,
               ULONG Key,
               BOOLEAN FailImmediately,
               BOOLEAN ExclusiveLock,
               OUT PIO_STATUS_BLOCK IoStatus,
               IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoLock\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoUnlockSingle(IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN PLARGE_INTEGER Length,
                       PEPROCESS ProcessId,
                       ULONG Key,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoUnlockSingle\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoUnlockAll(IN PFILE_OBJECT FileObject,
                    PEPROCESS ProcessId,
                    OUT PIO_STATUS_BLOCK IoStatus,
                    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoUnlockAll\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoUnlockAllByKey(IN PFILE_OBJECT FileObject,
                         PVOID ProcessId,
                         ULONG Key,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoUnlockAllByKey\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatFastIoQueryNetworkOpenInfo(IN PFILE_OBJECT FileObject,
                               IN BOOLEAN Wait,
                               OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
                               OUT PIO_STATUS_BLOCK IoStatus,
                               IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatFastIoQueryNetworkOpenInfo\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatMdlRead(IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN ULONG Length,
            IN ULONG LockKey,
            OUT PMDL* MdlChain,
            OUT PIO_STATUS_BLOCK IoStatus,
            IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatMdlRead\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatMdlReadComplete(IN PFILE_OBJECT FileObject,
                    IN PMDL MdlChain,
                    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatMdlReadComplete\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatPrepareMdlWrite(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN ULONG Length,
                    IN ULONG LockKey,
                    OUT PMDL* MdlChain,
                    OUT PIO_STATUS_BLOCK IoStatus,
                    IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatPrepareMdlWrite\n");
    return FALSE;
}

BOOLEAN
NTAPI
FatMdlWriteComplete(IN PFILE_OBJECT FileObject,
                     IN PLARGE_INTEGER FileOffset,
                     IN PMDL MdlChain,
                     IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatMdlWriteComplete\n");
    return FALSE;
}

NTSTATUS
NTAPI
FatAcquireForCcFlush(IN PFILE_OBJECT FileObject,
                      IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatAcquireForCcFlush\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FatReleaseForCcFlush(IN PFILE_OBJECT FileObject,
                      IN PDEVICE_OBJECT DeviceObject)
{
    DPRINT("FatReleaseForCcFlush\n");
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
FatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait)
{
    DPRINT("FatAcquireForLazyWrite()\n");
    return FALSE;
}

VOID
NTAPI
FatReleaseFromLazyWrite(IN PVOID Context)
{
    DPRINT("FatReleaseFromLazyWrite()\n");
}

BOOLEAN
NTAPI
FatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait)
{
    DPRINT("FatAcquireForReadAhead()\n");
    return FALSE;
}

VOID
NTAPI
FatReleaseFromReadAhead(IN PVOID Context)
{
    DPRINT("FatReleaseFromReadAhead()\n");
}

BOOLEAN
NTAPI
FatNoopAcquire(IN PVOID Context,
               IN BOOLEAN Wait)
{
    return TRUE;
}

VOID
NTAPI
FatNoopRelease(IN PVOID Context)
{
}


VOID
FatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch)
{
    /* Set Fast I/O dispatcher callbacks */
    FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch->FastIoCheckIfPossible = FatFastIoCheckIfPossible;
    FastIoDispatch->FastIoRead = FatFastIoRead;
    FastIoDispatch->FastIoWrite = FatFastIoWrite;
    FastIoDispatch->FastIoQueryBasicInfo = FatFastIoQueryBasicInfo;
    FastIoDispatch->FastIoQueryStandardInfo = FatFastIoQueryStandardInfo;
    FastIoDispatch->FastIoLock = FatFastIoLock;
    FastIoDispatch->FastIoUnlockSingle = FatFastIoUnlockSingle;
    FastIoDispatch->FastIoUnlockAll = FatFastIoUnlockAll;
    FastIoDispatch->FastIoUnlockAllByKey = FatFastIoUnlockAllByKey;
    FastIoDispatch->FastIoQueryNetworkOpenInfo = FatFastIoQueryNetworkOpenInfo;
    FastIoDispatch->MdlRead = FatMdlRead;
    FastIoDispatch->MdlReadComplete = FatMdlReadComplete;
    FastIoDispatch->PrepareMdlWrite = FatPrepareMdlWrite;
    FastIoDispatch->MdlWriteComplete = FatMdlWriteComplete;
    FastIoDispatch->AcquireForCcFlush = FatAcquireForCcFlush;
    FastIoDispatch->ReleaseForCcFlush = FatReleaseForCcFlush;
}

/* EOF */

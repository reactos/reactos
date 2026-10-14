

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/* Writes what the cache still holds for this file */
static
NTSTATUS
NTAPI
NtfsFlushFileCache(
    _In_ PNTFS_FCB Fcb)
{
    IO_STATUS_BLOCK IoStatus;

    if ((Fcb->Flags & FCB_IS_VOLUME) || Fcb->SectionObjectPointers.DataSectionObject == NULL)
        return STATUS_SUCCESS;

    /* Keep cached writes out so the flush covers all of them */
    ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
    CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, &IoStatus);
    ExReleaseResourceLite(&Fcb->MainResource);

    return IoStatus.Status;
}

/* Flushes every file of the volume, as a flush of the volume handle asks for */
static
NTSTATUS
NTAPI
NtfsFlushVolumeFiles(
    _In_ PDEVICE_EXTENSION DeviceExt)
{
    PNTFS_FCB *Fcbs;
    PLIST_ENTRY Entry;
    ULONG Count = 0, Taken = 0, i;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;

    KeAcquireSpinLock(&DeviceExt->FcbListLock, &OldIrql);
    for (Entry = DeviceExt->FcbListHead.Flink; Entry != &DeviceExt->FcbListHead; Entry = Entry->Flink)
        Count++;
    KeReleaseSpinLock(&DeviceExt->FcbListLock, OldIrql);

    if (Count == 0)
        return STATUS_SUCCESS;

    /* Some slack for files opened meanwhile, the next flush gets the rest */
    Count += 16;
    Fcbs = ExAllocatePoolWithTag(NonPagedPool, Count * sizeof(*Fcbs), TAG_NTFS);
    if (Fcbs == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Take references, a delete may take an FCB off the list while we flush.
     * Unreferenced FCBs only linger for their section and have nothing cached
     * to write; releasing one again would tear it down twice. */
    KeAcquireSpinLock(&DeviceExt->FcbListLock, &OldIrql);
    for (Entry = DeviceExt->FcbListHead.Flink;
         Entry != &DeviceExt->FcbListHead && Taken < Count;
         Entry = Entry->Flink)
    {
        PNTFS_FCB Fcb = CONTAINING_RECORD(Entry, NTFS_FCB, FcbListEntry);

        if (Fcb->RefCount <= 0)
            continue;

        Fcb->RefCount++;
        Fcbs[Taken++] = Fcb;
    }
    KeReleaseSpinLock(&DeviceExt->FcbListLock, OldIrql);

    for (i = 0; i < Taken; i++)
    {
        NTSTATUS FlushStatus = NtfsFlushFileCache(Fcbs[i]);

        if (!NT_SUCCESS(FlushStatus) && NT_SUCCESS(Status))
            Status = FlushStatus;

        NtfsReleaseFCB(DeviceExt, Fcbs[i]);
    }

    ExFreePoolWithTag(Fcbs, TAG_NTFS);

    return Status;
}

/**
* @name NtfsFlushBuffers
* @implemented
*
* Handles IRP_MJ_FLUSH_BUFFERS.
*
* @param IrpContext
* Points to an NTFS_IRP_CONTEXT which describes the request
*
* @return
* STATUS_SUCCESS if everything reached the media, otherwise the status the
* storage stack returned.
*
*/
NTSTATUS
NtfsFlushBuffers(PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    PNTFS_FCB Fcb;
    NTSTATUS Status;
    NTSTATUS DeviceStatus;

    DPRINT("NtfsFlushBuffers(%p)\n", IrpContext);

    IrpContext->Irp->IoStatus.Information = 0;

    /* Nothing is mounted on the main device object */
    if (IrpContext->DeviceObject == NtfsGlobalData->DeviceObject)
    {
        return STATUS_SUCCESS;
    }

    DeviceExt = IrpContext->DeviceObject->DeviceExtension;

    if (DeviceExt->Flags & VCB_VOLUME_DISMOUNTED)
    {
        return STATUS_VOLUME_DISMOUNTED;
    }

    /* Flushing waits on the disk */
    if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
        return NtfsMarkIrpContextForQueue(IrpContext);

    /* Cached writes only reach the disk once the cache lets them go */
    Fcb = (PNTFS_FCB)IrpContext->FileObject->FsContext;
    if (NtfsIsRegularFileObject(IrpContext->FileObject))
        Status = NtfsFlushFileCache(Fcb);
    else if (Fcb != NULL && Fcb->Identifier.Type == NTFS_TYPE_FCB && (Fcb->Flags & FCB_IS_VOLUME))
        Status = NtfsFlushVolumeFiles(DeviceExt);
    else
        Status = STATUS_SUCCESS;

    DeviceStatus = NtfsFlushDevice(DeviceExt->StorageDevice);

    return NT_SUCCESS(Status) ? DeviceStatus : Status;
}

/* EOF */

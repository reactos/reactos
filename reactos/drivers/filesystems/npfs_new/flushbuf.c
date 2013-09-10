#include "npfs.h"

NTSTATUS
NTAPI
NpCommonFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NODE_TYPE_CODE NodeTypeCode;
    PNP_CCB Ccb;
    BOOLEAN ServerSide;
    NTSTATUS Status;
    PNP_DATA_QUEUE FlushQueue;
    PAGED_CODE();

    NodeTypeCode = NpDecodeFileObject(IoGetCurrentIrpStackLocation(Irp)->FileObject,
                                      NULL,
                                      &Ccb,
                                      &ServerSide);
    if (NodeTypeCode != NPFS_NTC_CCB ) return STATUS_PIPE_DISCONNECTED;

    ExAcquireResourceExclusiveLite(&Ccb->NonPagedCcb->Lock, TRUE);
    //ms_exc.registration.TryLevel = 0;

    FlushQueue = &Ccb->OutQueue;
    if ( ServerSide != 1 ) FlushQueue = &Ccb->InQueue;

    if ( FlushQueue->QueueState == WriteEntries)
    {
        Status = NpAddDataQueueEntry(ServerSide,
                                     Ccb,
                                     FlushQueue,
                                     1,
                                     2u,
                                     0,
                                     Irp,
                                     NULL,
                                     0);
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

   // ms_exc.registration.TryLevel = -1;
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);

    return Status;
}

NTSTATUS
NTAPI
NpFsdFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();
    ExAcquireResourceExclusiveLite(&NpVcb->Lock, TRUE);

    Status = NpCommonFlushBuffers(DeviceObject, Irp);

    ExReleaseResourceLite(&NpVcb->Lock);
    FsRtlExitFileSystem();

    return Status;
}


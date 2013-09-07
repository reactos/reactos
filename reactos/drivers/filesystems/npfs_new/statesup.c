#include "npfs.h"

NTSTATUS
NTAPI
NpSetConnectedPipeState(IN PNP_CCB Ccb,
                        IN PFILE_OBJECT FileObject,
                        IN PLIST_ENTRY List)
{
    PLIST_ENTRY NextEntry;
    PIRP Irp;

    ASSERT(Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE);

    Ccb->ClientReadMode = FILE_PIPE_BYTE_STREAM_MODE;
    Ccb->ClientCompletionMode = FILE_PIPE_QUEUE_OPERATION;
    Ccb->NamedPipeState = FILE_PIPE_CONNECTED_STATE;
    Ccb->ClientFileObject = FileObject;

    NpSetFileObject(FileObject, Ccb, Ccb->NonPagedCcb, FALSE);

    while (!IsListEmpty(&Ccb->IrpList))
    {
        NextEntry = RemoveHeadList(&Ccb->IrpList);

        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

        if (IoSetCancelRoutine(Irp, NULL))
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            InsertTailList(List, NextEntry);
        }
        else
        {
            InitializeListHead(NextEntry);
        }
    }

    return STATUS_SUCCESS;
}

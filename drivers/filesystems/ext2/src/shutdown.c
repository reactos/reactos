/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             shutdown.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2ShutDown)
#endif

NTSTATUS
Ext2ShutDown (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS                Status;

    PIRP                    Irp;

    PEXT2_VCB               Vcb;
    PLIST_ENTRY             ListEntry;

    BOOLEAN                 GlobalResourceAcquired = FALSE;

    _SEH2_TRY {

        Status = STATUS_SUCCESS;

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        Irp = IrpContext->Irp;

        if (!ExAcquireResourceExclusiveLite(
                    &Ext2Global->Resource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }

        GlobalResourceAcquired = TRUE;

        for (ListEntry = Ext2Global->VcbList.Flink;
                ListEntry != &(Ext2Global->VcbList);
                ListEntry = ListEntry->Flink ) {

            Vcb = CONTAINING_RECORD(ListEntry, EXT2_VCB, Next);

            if (ExAcquireResourceExclusiveLite(
                        &Vcb->MainResource,
                        TRUE )) {

                if (IsMounted(Vcb)) {

                    /* update mount count */
                    Vcb->SuperBlock->s_mnt_count++;
                    if (Vcb->SuperBlock->s_mnt_count >
                            Vcb->SuperBlock->s_max_mnt_count ) {
                        Vcb->SuperBlock->s_mnt_count =
                            Vcb->SuperBlock->s_max_mnt_count;
                    }
                    Ext2SaveSuper(IrpContext, Vcb);

                    /* flush dirty cache for all files */
                    Status = Ext2FlushFiles(IrpContext, Vcb, TRUE);
                    if (!NT_SUCCESS(Status)) {
                        DbgBreak();
                    }

                    /* flush volume stream's cache to disk */
                    Status = Ext2FlushVolume(IrpContext, Vcb, TRUE);

                    if (!NT_SUCCESS(Status) && Status != STATUS_MEDIA_WRITE_PROTECTED) {
                        DbgBreak();
                    }

                    /* send shutdown request to underlying disk */
                    Ext2DiskShutDown(Vcb);
                }

                ExReleaseResourceLite(&Vcb->MainResource);
            }
        }

        /*
                IoUnregisterFileSystem(Ext2Global->DiskdevObject);
                IoUnregisterFileSystem(Ext2Global->CdromdevObject);
        */

    } _SEH2_FINALLY {

        if (GlobalResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}
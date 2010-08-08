/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/close.c
 * PURPOSE:         Closing routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatiCommonClose(IN PVCB Vcb,
                IN PFCB Fcb,
                IN PCCB Ccb,
                IN TYPE_OF_OPEN TypeOfOpen,
                IN BOOLEAN Wait,
                OUT PBOOLEAN VcbDeleted)
{
    NTSTATUS Status;
    PFCB ParentDcb;
    BOOLEAN RecursiveClose, VcbDeletedLv = FALSE;
    FAT_IRP_CONTEXT IrpContext;

    if (VcbDeleted) *VcbDeleted = FALSE;

    if (TypeOfOpen == UnopenedFileObject)
    {
        DPRINT1("Closing unopened file object\n");
        Status = STATUS_SUCCESS;
        return Status;
    }

    RtlZeroMemory(&IrpContext, sizeof(FAT_IRP_CONTEXT));

    IrpContext.NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IrpContext);
    IrpContext.MajorFunction = IRP_MJ_CLOSE;

    if (Wait) SetFlag(IrpContext.Flags, IRPCONTEXT_CANWAIT);

    if (!ExAcquireResourceExclusiveLite(&Vcb->Resource, Wait)) return STATUS_PENDING;

    if (Vcb->State & VCB_STATE_FLAG_CLOSE_IN_PROGRESS)
    {
        RecursiveClose = TRUE;
    }
    else
    {
        SetFlag(Vcb->State, VCB_STATE_FLAG_CLOSE_IN_PROGRESS);
        RecursiveClose = FALSE;

        Vcb->OpenFileCount++;
    }

    /* Update on-disk structures */
    switch (TypeOfOpen)
    {
    case VirtualVolumeFile:
        DPRINT1("Close VirtualVolumeFile\n");

        InterlockedDecrement((PLONG)&(Vcb->InternalOpenCount));
        InterlockedDecrement((PLONG)&(Vcb->ResidualOpenCount));

        Status = STATUS_SUCCESS;
        goto close_done;
        break;

    case UserVolumeOpen:
        DPRINT1("Close UserVolumeOpen\n");

        Vcb->DirectAccessOpenCount--;
        Vcb->OpenFileCount--;
        if (FlagOn(Ccb->Flags, CCB_READ_ONLY)) Vcb->ReadOnlyCount--;

        FatDeleteCcb(&IrpContext, Ccb);

        Status = STATUS_SUCCESS;
        goto close_done;
        break;

    case EaFile:
        UNIMPLEMENTED;
        break;

    case DirectoryFile:
        DPRINT1("Close DirectoryFile\n");

        InterlockedDecrement((PLONG)&(Fcb->Dcb.DirectoryFileOpenCount));
        InterlockedDecrement((PLONG)&(Vcb->InternalOpenCount));

        if (FatNodeType(Fcb) == FAT_NTC_ROOT_DCB)
        {
            InterlockedDecrement((PLONG)&(Vcb->ResidualOpenCount));
        }

        if (RecursiveClose)
        {
            Status = STATUS_SUCCESS;
            goto close_done;
        }
        else
        {
            break;
        }

    case UserDirectoryOpen:
    case UserFileOpen:
        DPRINT("Close UserFileOpen/UserDirectoryOpen\n");

        if ((FatNodeType(Fcb) == FAT_NTC_DCB) &&
            IsListEmpty(&Fcb->Dcb.ParentDcbList) &&
            (Fcb->OpenCount == 1) &&
            (Fcb->Dcb.DirectoryFile != NULL))
        {
                PFILE_OBJECT DirectoryFileObject = Fcb->Dcb.DirectoryFile;

                DPRINT1("Uninitialize the stream file object\n");

                CcUninitializeCacheMap(DirectoryFileObject, NULL, NULL);

                Fcb->Dcb.DirectoryFile = NULL;
                ObDereferenceObject(DirectoryFileObject);
        }

        Fcb->OpenCount--;
        Vcb->OpenFileCount--;
        if (FlagOn(Ccb->Flags, CCB_READ_ONLY)) Vcb->ReadOnlyCount --;

        FatDeleteCcb(&IrpContext, Ccb);
        break;

    default:
        KeBugCheckEx(FAT_FILE_SYSTEM, __LINE__, (ULONG_PTR)TypeOfOpen, 0, 0);
    }

    /* Update in-memory structures */
    if (((FatNodeType(Fcb) == FAT_NTC_FCB) &&
        (Fcb->OpenCount == 0))
        ||
        ((FatNodeType(Fcb) == FAT_NTC_DCB) &&
        (IsListEmpty(&Fcb->Dcb.ParentDcbList)) &&
        (Fcb->OpenCount == 0) &&
        (Fcb->Dcb.DirectoryFileOpenCount == 0)))
    {
        ParentDcb = Fcb->ParentFcb;

        SetFlag(Vcb->State, VCB_STATE_FLAG_DELETED_FCB);

        FatDeleteFcb(&IrpContext, Fcb);

        while ((FatNodeType(ParentDcb) == FAT_NTC_DCB) &&
            IsListEmpty(&ParentDcb->Dcb.ParentDcbList) &&
            (ParentDcb->OpenCount == 0) &&
            (ParentDcb->Dcb.DirectoryFile != NULL))
        {
                PFILE_OBJECT DirectoryFileObject;

                DirectoryFileObject = ParentDcb->Dcb.DirectoryFile;

                DPRINT1("Uninitialize parent Stream Cache Map\n");

                CcUninitializeCacheMap(DirectoryFileObject, NULL, NULL);

                ParentDcb->Dcb.DirectoryFile = NULL;

                ObDereferenceObject(DirectoryFileObject);

                if (ParentDcb->Dcb.DirectoryFileOpenCount == 0)
                {
                    PFCB CurrentDcb;

                    CurrentDcb = ParentDcb;
                    ParentDcb = CurrentDcb->ParentFcb;

                    SetFlag(Vcb->State, VCB_STATE_FLAG_DELETED_FCB);

                    FatDeleteFcb(&IrpContext, CurrentDcb);
                }
                else
                {
                    break;
                }
        }
    }

    Status = STATUS_SUCCESS;

close_done:
    /* Closing is done, check if VCB could be closed too */
    if (!RecursiveClose)
    {
        /* One open left - yes, VCB can go away */
        if (Vcb->OpenFileCount == 1 &&
            !FlagOn(Vcb->State, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS)
            && VcbDeleted)
        {
            FatReleaseVcb(&IrpContext, Vcb );

            SetFlag(IrpContext.Flags, IRPCONTEXT_CANWAIT);

            FatAcquireExclusiveGlobal(&IrpContext);

            FatAcquireExclusiveVcb(&IrpContext, Vcb);

            Vcb->OpenFileCount--;

            VcbDeletedLv = FatCheckForDismount(&IrpContext, Vcb, FALSE);

            FatReleaseGlobal(&IrpContext);

            if (VcbDeleted) *VcbDeleted = VcbDeletedLv;
        }
        else
        {
            /* Remove extra referenec */
            Vcb->OpenFileCount --;
        }

        /* Clear recursion flag if necessary */
        if (!VcbDeletedLv)
        {
            ClearFlag(Vcb->State, VCB_STATE_FLAG_CLOSE_IN_PROGRESS);
        }
    }

    /* Release VCB if it wasn't deleted */
    if (!VcbDeletedLv)
        FatReleaseVcb(&IrpContext, Vcb);

    return Status;
}

NTSTATUS
NTAPI
FatiClose(IN PFAT_IRP_CONTEXT IrpContext,
          IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    BOOLEAN TopLevel, Wait, VcbDeleted = FALSE;
    NTSTATUS Status;
    PCLOSE_CONTEXT CloseContext = NULL;

    TopLevel = FatIsTopLevelIrp(Irp);

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Decode incoming file object */
    TypeOfOpen = FatDecodeFileObject(IrpSp->FileObject, &Vcb, &Fcb, &Ccb);

    /* Set CCB read only flag */
    if (Ccb && IsFileObjectReadOnly(IrpSp->FileObject))
        SetFlag(Ccb->Flags, CCB_READ_ONLY);

    /* It's possible to wait only if we are top level or not a system process */
    Wait = TopLevel && (PsGetCurrentProcess() != FatGlobalData.SystemProcess);

    /* Call the common handler */
    Status = FatiCommonClose(Vcb, Fcb, Ccb, TypeOfOpen, Wait, &VcbDeleted);

    if (((TypeOfOpen == UserFileOpen ||
        TypeOfOpen == UserDirectoryOpen) &&
        (Fcb->State & FCB_STATE_DELAY_CLOSE) &&
        !FatGlobalData.ShutdownStarted) ||
        Status == STATUS_PENDING)
    {
        DPRINT1("TODO: Queue a pending close request\n");
    }
    else
    {
        /* Close finished right away */
        if (TypeOfOpen == VirtualVolumeFile ||
            TypeOfOpen == DirectoryFile ||
            TypeOfOpen == EaFile)
        {
                if (TypeOfOpen == VirtualVolumeFile)
                {
                    /* Free close context for the not deleted VCB */
                    if (!VcbDeleted)
                    {
                        CloseContext = Vcb->CloseContext;
                        Vcb->CloseContext = NULL;

                        ASSERT(CloseContext != NULL);
                    }
                }
                else
                {
                    //CloseContext = FatAllocateCloseContext(Vcb);
                    DPRINT1("TODO: Allocate close context!\n");
                    ASSERT(CloseContext != NULL);
                }

                /* Free close context */
                if (CloseContext) ExFreePool(CloseContext);
        }
    }

    /* Complete the request */
    FatCompleteRequest(NULL, Irp, Status);

    /* Reset the top level IRP if necessary */
    if (TopLevel) IoSetTopLevelIrp(NULL);

    return Status;
}

NTSTATUS
NTAPI
FatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFAT_IRP_CONTEXT IrpContext;
    NTSTATUS Status;

    DPRINT("FatClose(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    /* FatClose works only with a volume device object */
    if (DeviceObject == FatGlobalData.DiskDeviceObject)
    {
        /* Complete the request and return success */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        return STATUS_SUCCESS;
    }

    /* Enter FsRtl critical region */
    FsRtlEnterFileSystem();

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, TRUE);

    /* Call internal function */
    Status = FatiClose(IrpContext, Irp);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

/* EOF */

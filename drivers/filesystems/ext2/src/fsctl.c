/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fsctl.c
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
#pragma alloc_text(PAGE, Ext2IsHandleCountZero)
#pragma alloc_text(PAGE, Ext2LockVcb)
#pragma alloc_text(PAGE, Ext2LockVolume)
#pragma alloc_text(PAGE, Ext2UnlockVcb)
#pragma alloc_text(PAGE, Ext2UnlockVolume)
#pragma alloc_text(PAGE, Ext2AllowExtendedDasdIo)
#pragma alloc_text(PAGE, Ext2GetRetrievalPointerBase)
#pragma alloc_text(PAGE, Ext2QueryExtentMappings)
#pragma alloc_text(PAGE, Ext2QueryRetrievalPointers)
#pragma alloc_text(PAGE, Ext2GetRetrievalPointers)
#pragma alloc_text(PAGE, Ext2UserFsRequest)
#pragma alloc_text(PAGE, Ext2IsMediaWriteProtected)
#pragma alloc_text(PAGE, Ext2MountVolume)
#pragma alloc_text(PAGE, Ext2PurgeVolume)
#pragma alloc_text(PAGE, Ext2PurgeFile)
#pragma alloc_text(PAGE, Ext2DismountVolume)
#pragma alloc_text(PAGE, Ext2IsVolumeMounted)
#pragma alloc_text(PAGE, Ext2VerifyVolume)
#pragma alloc_text(PAGE, Ext2FileSystemControl)
#endif


VOID
Ext2SetVpbFlag (
    IN PVPB     Vpb,
    IN USHORT   Flag )
{
    KIRQL OldIrql;

    IoAcquireVpbSpinLock(&OldIrql);
    Vpb->Flags |= Flag;
    IoReleaseVpbSpinLock(OldIrql);
}

VOID
Ext2ClearVpbFlag (
    IN PVPB     Vpb,
    IN USHORT   Flag )
{
    KIRQL OldIrql;

    IoAcquireVpbSpinLock(&OldIrql);
    Vpb->Flags &= ~Flag;
    IoReleaseVpbSpinLock(OldIrql);
}

BOOLEAN
Ext2IsHandleCountZero(IN PEXT2_VCB Vcb)
{
    PEXT2_FCB   Fcb;
    PLIST_ENTRY List;

    for ( List = Vcb->FcbList.Flink;
            List != &Vcb->FcbList;
            List = List->Flink )  {

        Fcb = CONTAINING_RECORD(List, EXT2_FCB, Next);

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        DEBUG(DL_INF, ( "Ext2IsHandleCountZero: Inode:%xh File:%S OpenHandleCount=%xh\n",
                        Fcb->Inode->i_ino, Fcb->Mcb->ShortName.Buffer, Fcb->OpenHandleCount));

        if (Fcb->OpenHandleCount) {
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS
Ext2LockVcb (IN PEXT2_VCB    Vcb,
             IN PFILE_OBJECT FileObject)
{
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY {

        if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            DEBUG(DL_INF, ( "Ext2LockVolume: Volume is already locked.\n"));
            Status = STATUS_ACCESS_DENIED;
            _SEH2_LEAVE;
        }

        if (Vcb->OpenHandleCount > (ULONG)(FileObject ? 1 : 0)) {
            DEBUG(DL_INF, ( "Ext2LockVcb: There are still opened files.\n"));

            Status = STATUS_ACCESS_DENIED;
            _SEH2_LEAVE;
        }

        if (!Ext2IsHandleCountZero(Vcb)) {
            DEBUG(DL_INF, ( "Ext2LockVcb: Thare are still opened files.\n"));

            Status = STATUS_ACCESS_DENIED;
            _SEH2_LEAVE;
        }

        SetLongFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        Ext2SetVpbFlag(Vcb->Vpb, VPB_LOCKED);
        Vcb->LockFile = FileObject;

        DEBUG(DL_INF, ( "Ext2LockVcb: Volume locked.\n"));

    } _SEH2_FINALLY {
        // Nothing
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2LockVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT  DeviceObject;
    PEXT2_VCB       Vcb = NULL;
    NTSTATUS        Status;
    BOOLEAN VcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        Status = STATUS_UNSUCCESSFUL;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));

        IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

#if (_WIN32_WINNT >= 0x0500)
        CcWaitForCurrentLazyWriterActivity();
#endif
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE            );

        VcbResourceAcquired = TRUE;

        /* flush dirty data before locking the volume */
        if (!IsVcbReadOnly(Vcb)) {
            Ext2FlushFiles(IrpContext, Vcb, FALSE);
            Ext2FlushVolume(IrpContext, Vcb, FALSE);
        }

        Status = Ext2LockVcb(Vcb, IrpSp->FileObject);

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2UnlockVcb ( IN PEXT2_VCB    Vcb,
                IN PFILE_OBJECT FileObject )
{
    NTSTATUS        Status;

    _SEH2_TRY {

        if (FileObject && FileObject->FsContext != Vcb) {
            Status = STATUS_NOT_LOCKED;
            _SEH2_LEAVE;
        }

        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            DEBUG(DL_ERR, ( ": Ext2UnlockVcb: Volume is not locked.\n"));
            Status = STATUS_NOT_LOCKED;
            _SEH2_LEAVE;
        }

        if (Vcb->LockFile == FileObject) {
            ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
            Ext2ClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
            DEBUG(DL_INF, ( "Ext2UnlockVcb: Volume unlocked.\n"));
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NOT_LOCKED;
        }

    } _SEH2_FINALLY {
        // Nothing
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2UnlockVolume (
    IN PEXT2_IRP_CONTEXT IrpContext
)
{
    PIO_STACK_LOCATION IrpSp = NULL;
    PDEVICE_OBJECT  DeviceObject = NULL;
    PEXT2_VCB       Vcb = NULL;
    NTSTATUS        Status;
    BOOLEAN         VcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        VcbResourceAcquired = TRUE;

        Status = Ext2UnlockVcb(Vcb, IrpSp->FileObject);

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2InvalidateVolumes ( IN PEXT2_IRP_CONTEXT IrpContext )
{
    NTSTATUS            Status;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;

#ifndef __REACTOS__
    PVPB                NewVpb = NULL;
#endif
    HANDLE              Handle;
    PLIST_ENTRY         ListEntry;

    ULONG               InputLength = 0;
    PFILE_OBJECT        FileObject;
    PDEVICE_OBJECT      DeviceObject;
    BOOLEAN             GlobalResourceAcquired = FALSE;

    LUID Privilege = {SE_TCB_PRIVILEGE, 0};

    _SEH2_TRY {

        Irp   = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (!IsExt2FsDevice(IrpSp->DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        if (!SeSinglePrivilegeCheck(Privilege, Irp->RequestorMode)) {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            _SEH2_LEAVE;
        }


#ifndef _GNU_NTIFS_
        InputLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
#else
        InputLength = ((PEXTENDED_IO_STACK_LOCATION)(IrpSp))->
                      Parameters.FileSystemControl.InputBufferLength;
#endif

#if defined(_WIN64)
        if (IoIs32bitProcess(Irp)) {
            if (InputLength != sizeof(UINT32)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
            Handle = (HANDLE) LongToHandle( (*(PUINT32)Irp->AssociatedIrp.SystemBuffer) );
        } else
#endif
        {
            if (InputLength != sizeof(HANDLE)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
            Handle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
        }

        Status = ObReferenceObjectByHandle( Handle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (void **)&FileObject,
                                            NULL );

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        } else {
            DeviceObject = FileObject->DeviceObject;
            ObDereferenceObject(FileObject);
        }

        ExAcquireResourceExclusiveLite(&Ext2Global->Resource,  TRUE);
        GlobalResourceAcquired = TRUE;

        ListEntry = Ext2Global->VcbList.Flink;
        while (ListEntry != &Ext2Global->VcbList)  {

            PEXT2_VCB Vcb = CONTAINING_RECORD(ListEntry, EXT2_VCB, Next);
            ListEntry = ListEntry->Flink;

            DEBUG(DL_DBG, ( "Ext2InvalidateVolumes: Vcb=%xh Vcb->Vpb=%xh "
                            "Blink = %p &Vcb->Next = %p\n",
                            Vcb, Vcb->Vpb, ListEntry->Blink, &Vcb->Next));

            if (Vcb->Vpb && (Vcb->Vpb->RealDevice == DeviceObject)) {

                DEBUG(DL_DBG, ( "Ext2InvalidateVolumes: Got Vcb=%xh Vcb->Vpb=%xh "
                                "Blink = %p &Vcb->Next = %p\n",
                                Vcb, Vcb->Vpb, ListEntry->Blink, &Vcb->Next));
                /* dismount the volume */
                Ext2CheckDismount(IrpContext, Vcb, FALSE);
            }
        }

    } _SEH2_FINALLY {

        if (GlobalResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2AllowExtendedDasdIo(IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIO_STACK_LOCATION IrpSp;
    PEXT2_VCB Vcb;
    PEXT2_CCB Ccb;
    NTSTATUS  status;

    IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

    Vcb = (PEXT2_VCB) IrpSp->FileObject->FsContext;
    Ccb = (PEXT2_CCB) IrpSp->FileObject->FsContext2;

    ASSERT(Vcb != NULL);

    ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
           (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

    ASSERT(IsMounted(Vcb));

    if (Ccb) {
        SetLongFlag(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO);
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    Ext2CompleteIrpContext(IrpContext, status);
    return status;
}

/*
 *  Ext2OplockRequest
 *
 *    oplock requests handler routine
 *
 *  Arguments:
 *    IrpContext: the ext2 irp context
 *
 *  Return Value:
 *    NTSTATUS:  The return status for the operation
 *
 */

NTSTATUS
Ext2OplockRequest (
    IN PEXT2_IRP_CONTEXT IrpContext
)
{
    NTSTATUS    Status;

    ULONG       FsCtrlCode;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT   FileObject;

    PIRP Irp = NULL;
    PIO_STACK_LOCATION IrpSp;
    PEXTENDED_IO_STACK_LOCATION EIrpSp;

    PEXT2_VCB   Vcb = NULL;
    PEXT2_FCB   Fcb = NULL;
    PEXT2_CCB   Ccb = NULL;

    ULONG OplockCount = 0;

    BOOLEAN VcbResourceAcquired = FALSE;
    BOOLEAN FcbResourceAcquired = FALSE;

    ASSERT(IrpContext);

    _SEH2_TRY {

        Irp = IrpContext->Irp;
        ASSERT(Irp);

        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);
        EIrpSp = (PEXTENDED_IO_STACK_LOCATION)IrpSp;

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;

        Fcb = (PEXT2_FCB) FileObject->FsContext;

        //
        // This request is not allowed on volumes
        //

        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Mcb->Flags, MCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }


        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        FsCtrlCode = EIrpSp->Parameters.FileSystemControl.FsControlCode;

        switch (FsCtrlCode) {

        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
        case FSCTL_REQUEST_BATCH_OPLOCK:

            VcbResourceAcquired =
                ExAcquireResourceSharedLite(
                    &Vcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

            ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

            FcbResourceAcquired =
                ExAcquireResourceExclusiveLite (
                    &Fcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

            if (FsCtrlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {
                OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks(&Fcb->FileLockAnchor);
            } else {
                OplockCount = Fcb->OpenHandleCount;
            }

            break;

        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
        case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
        case FSCTL_OPLOCK_BREAK_NOTIFY:
        case FSCTL_OPLOCK_BREAK_ACK_NO_2:

            FcbResourceAcquired =
                ExAcquireResourceSharedLite (
                    &Fcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

            break;

        default:

            Ext2BugCheck(EXT2_BUGCHK_FSCTL, FsCtrlCode, 0, 0);
        }


        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Fcb->Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);
        IrpContext->Irp = NULL;

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!_SEH2_AbnormalTermination()) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2IsVolumeDirty (
    IN PEXT2_IRP_CONTEXT IrpContext
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIRP  Irp;
    PEXTENDED_IO_STACK_LOCATION IrpSp;
    PULONG VolumeState;

    _SEH2_TRY {

        Irp = IrpContext->Irp;
        IrpSp = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);

        //
        //  Get a pointer to the output buffer.  Look at the system buffer field in th
        //  irp first.  Then the Irp Mdl.
        //

        if (Irp->AssociatedIrp.SystemBuffer != NULL) {

            VolumeState = Irp->AssociatedIrp.SystemBuffer;

        } else if (Irp->MdlAddress != NULL) {

            VolumeState = MmGetSystemAddressForMdl( Irp->MdlAddress );

        } else {

            status = STATUS_INVALID_USER_BUFFER;
            _SEH2_LEAVE;
        }

        if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {
            status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        *VolumeState = 0;

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  status);
        }
    } _SEH2_END;

    return status;
}


NTSTATUS
Ext2QueryExtentMappings(
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Fcb,
    IN PLARGE_INTEGER      RequestVbn,
    OUT PLARGE_INTEGER *   pMappedRuns
)
{
    PLARGE_INTEGER      MappedRuns = NULL;
    PLARGE_INTEGER      PartialRuns = NULL;

    PEXT2_EXTENT        Chain = NULL;
    PEXT2_EXTENT        Extent = NULL;

    LONGLONG            Vbn = 0;
    ULONG               Length = 0;
    ULONG               i = 0;

    NTSTATUS            Status = STATUS_SUCCESS;

    _SEH2_TRY {

        /* now building all the request extents */
        while (Vbn < RequestVbn->QuadPart) {

            Length = 0x80000000; /* 2g bytes */
            if (RequestVbn->QuadPart < Vbn + Length) {
                Length = (ULONG)(RequestVbn->QuadPart - Vbn);
            }

            /* build extents for sub-range */
            Extent = NULL;
            Status = Ext2BuildExtents(
                         IrpContext,
                         Vcb,
                         Fcb->Mcb,
                         Vbn,
                         Length,
                         FALSE,
                         &Extent);

            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            if (Chain) {
                Ext2JointExtents(Chain, Extent);
            } else {
                Chain = Extent;
            }

            /* allocate extent array */
            PartialRuns = Ext2AllocatePool(
                              NonPagedPool,
                              (Ext2CountExtents(Chain) + 2) *
                              (2 * sizeof(LARGE_INTEGER)),
                              'RE2E');

            if (PartialRuns == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }
            RtlZeroMemory(  PartialRuns,
                            (Ext2CountExtents(Chain) + 2) *
                            (2 * sizeof(LARGE_INTEGER)));

            if (MappedRuns) {
                RtlMoveMemory(PartialRuns,
                              MappedRuns,
                              i * 2 * sizeof(LARGE_INTEGER));
                Ext2FreePool(MappedRuns, 'RE2E');
            }
            MappedRuns = PartialRuns;

            /* walk all the Mcb runs in Extent */
            for (; Extent != NULL; Extent = Extent->Next) {
                MappedRuns[i*2 + 0].QuadPart = Vbn + Extent->Offset;
                MappedRuns[i*2 + 1].QuadPart = Extent->Lba;
                i = i+1;
            }

            Vbn = Vbn + Length;
        }

        *pMappedRuns = MappedRuns;

    } _SEH2_FINALLY {

        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING) {
            if (MappedRuns) {
                Ext2FreePool(MappedRuns, 'RE2E');
            }
            *pMappedRuns = NULL;
        }

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2QueryRetrievalPointers (
    IN PEXT2_IRP_CONTEXT IrpContext
)
{
    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IrpSp;
    PEXTENDED_IO_STACK_LOCATION EIrpSp;

    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_CCB           Ccb = NULL;

    PLARGE_INTEGER      RequestVbn;
    PLARGE_INTEGER *    pMappedRuns;

    ULONG               InputSize;
    ULONG               OutputSize;

    NTSTATUS            Status = STATUS_SUCCESS;

    BOOLEAN FcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext);
        Irp = IrpContext->Irp;
        ASSERT(Irp);

        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        EIrpSp = (PEXTENDED_IO_STACK_LOCATION)IrpSp;
        ASSERT(IrpSp);

        InputSize = EIrpSp->Parameters.FileSystemControl.InputBufferLength;
        OutputSize = EIrpSp->Parameters.FileSystemControl.OutputBufferLength;

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        DbgBreak();

        /* This request is not allowed on the main device object */
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;

        /* check Fcb is valid or not */
        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        if (IsFlagOn(Fcb->Mcb->Flags, MCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        /* Is requstor in kernel and Fcb a paging file ? */
        if (Irp->RequestorMode != KernelMode ||
                !IsFlagOn(Fcb->Flags, FCB_PAGE_FILE) ||
                InputSize != sizeof(LARGE_INTEGER) ||
                OutputSize != sizeof(PVOID)) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite (
                    &Fcb->MainResource, Ext2CanIWait())) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        FcbResourceAcquired = TRUE;

        RequestVbn  = EIrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        pMappedRuns = Irp->UserBuffer;

        DbgBreak();

        /* request size beyonds whole file size */
        if (RequestVbn->QuadPart >= Fcb->Header.AllocationSize.QuadPart) {
            Status = STATUS_END_OF_FILE;
            _SEH2_LEAVE;
        }

        Status = Ext2QueryExtentMappings(
                     IrpContext,
                     Vcb,
                     Fcb,
                     RequestVbn,
                     pMappedRuns
                 );

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (!_SEH2_AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2GetRetrievalPointers (
    IN PEXT2_IRP_CONTEXT IrpContext
)
{
    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IrpSp;
    PEXTENDED_IO_STACK_LOCATION EIrpSp;

    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_CCB           Ccb = NULL;

    PSTARTING_VCN_INPUT_BUFFER  SVIB;
    PRETRIEVAL_POINTERS_BUFFER  RPSB;

    PEXT2_EXTENT        Chain = NULL;
    PEXT2_EXTENT        Extent = NULL;

    LONGLONG            Vbn = 0;
    ULONG               Length = 0;
    ULONG               i = 0;

    ULONG               UsedSize = 0;
    ULONG               InputSize;
    ULONG               OutputSize;

    NTSTATUS            Status = STATUS_SUCCESS;

    BOOLEAN FcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext);
        Irp = IrpContext->Irp;
        ASSERT(Irp);

        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        EIrpSp = (PEXTENDED_IO_STACK_LOCATION)IrpSp;
        ASSERT(IrpSp);

        InputSize = EIrpSp->Parameters.FileSystemControl.InputBufferLength;
        OutputSize = EIrpSp->Parameters.FileSystemControl.OutputBufferLength;

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        /* This request is not allowed on the main device object */
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;

        /* check Fcb is valid or not */
        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Mcb->Flags, MCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        if (InputSize  < sizeof(STARTING_VCN_INPUT_BUFFER) ||
                OutputSize < sizeof(RETRIEVAL_POINTERS_BUFFER) ) {
            Status = STATUS_BUFFER_TOO_SMALL;
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite (
                    &Fcb->MainResource, Ext2CanIWait())) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        FcbResourceAcquired = TRUE;

        SVIB = (PSTARTING_VCN_INPUT_BUFFER)
               EIrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        RPSB = (PRETRIEVAL_POINTERS_BUFFER) Ext2GetUserBuffer(Irp);

        /* probe user buffer */

        _SEH2_TRY {
            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead (SVIB, InputSize,  sizeof(UCHAR));
                ProbeForWrite(RPSB, OutputSize, sizeof(UCHAR));
            }
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_INVALID_USER_BUFFER;
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        UsedSize = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[0]);

        /* request size beyonds whole file size ? */
        DEBUG(DL_DBG, ("Ext2GetRetrievalPointers: Startin from Vbn: %I64xh\n",
                       SVIB->StartingVcn.QuadPart));
        Vbn = (SVIB->StartingVcn.QuadPart << BLOCK_BITS);
        if (Vbn >= Fcb->Header.AllocationSize.QuadPart ) {
            Status = STATUS_END_OF_FILE;
            _SEH2_LEAVE;
        }

        /* now building all the request extents */
        while (Vbn < Fcb->Header.AllocationSize.QuadPart) {

            ASSERT(Chain == NULL);
            Length = 0x80000000; /* 2g bytes */
            if (Fcb->Header.AllocationSize.QuadPart < Vbn + Length) {
                Length = (ULONG)(Fcb->Header.AllocationSize.QuadPart - Vbn);
            }

            /* build extents for sub-range */
            Status = Ext2BuildExtents(
                         IrpContext,
                         Vcb,
                         Fcb->Mcb,
                         Vbn,
                         Length,
                         FALSE,
                         &Chain);

            if (!NT_SUCCESS(Status)) {
                DbgBreak();
                _SEH2_LEAVE;
            }

            /* fill user buffer of RETRIEVAL_POINTERS_BUFFER */
            Extent = Chain;
            while (Extent) {

                DEBUG(DL_MAP, ("Ext2GetRetrievalPointers: %wZ %d Vbn = %I64xh Lbn = %I64xh\n",
                               &Fcb->Mcb->FullName, i,
                               ((Vbn + Extent->Offset) >> BLOCK_BITS),
                               Extent->Lba));

                RPSB->Extents[i].Lcn.QuadPart = (Extent->Lba >> BLOCK_BITS);
                RPSB->Extents[i].NextVcn.QuadPart = ((Vbn + Extent->Offset + Extent->Length) >> BLOCK_BITS);
                if (i == 0) {
                    RPSB->StartingVcn.QuadPart = ((Vbn + Extent->Offset) >> BLOCK_BITS);
                } else {
                    ASSERT(RPSB->Extents[i-1].NextVcn.QuadPart == ((Vbn + Extent->Offset) >> BLOCK_BITS));
                }
                if (UsedSize + sizeof(RETRIEVAL_POINTERS_BUFFER) > OutputSize) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    _SEH2_LEAVE;
                }
                UsedSize += sizeof(LARGE_INTEGER) * 2;
                Irp->IoStatus.Information = (ULONG_PTR)UsedSize;
                RPSB->ExtentCount = ++i;
                Extent = Extent->Next;
            }

            if (Chain) {
                Ext2DestroyExtentChain(Chain);
                Chain = NULL;
            }

            Vbn = Vbn + Length;
        }

#if 0
        {
            NTSTATUS _s;
            ULONG  _i = 0;
            LARGE_INTEGER RequestVbn = Fcb->Header.AllocationSize;
            PLARGE_INTEGER MappedRuns = NULL;

            _s = Ext2QueryExtentMappings(
                     IrpContext,
                     Vcb,
                     Fcb,
                     &RequestVbn,
                     &MappedRuns
                 );
            if (!NT_SUCCESS(_s) || NULL == MappedRuns) {
                DbgBreak();
                goto exit_to_get_rps;
            }

            while (MappedRuns[_i*2 + 0].QuadPart != 0 ||
                    MappedRuns[_i*2 + 1].QuadPart != 0 ) {
                DEBUG(DL_MAP, ("Ext2QueryExtentMappings: %wZ %d Vbn = %I64xh Lbn = %I64xh\n",
                               &Fcb->Mcb->FullName, _i,
                               MappedRuns[_i*2 + 0].QuadPart,
                               MappedRuns[_i*2 + 1].QuadPart));
                _i++;
            }

exit_to_get_rps:

            if (MappedRuns) {
                Ext2FreePool(MappedRuns, 'RE2E');
            }
        }
#endif

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }

        if (!_SEH2_AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2GetRetrievalPointerBase (
    IN PEXT2_IRP_CONTEXT IrpContext
)
{
    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IrpSp;
    PEXTENDED_IO_STACK_LOCATION EIrpSp;

    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_CCB           Ccb = NULL;

    PLARGE_INTEGER      FileAreaOffset;

    ULONG               OutputSize;

    NTSTATUS            Status = STATUS_SUCCESS;

    BOOLEAN FcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext);
        Irp = IrpContext->Irp;
        ASSERT(Irp);

        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        EIrpSp = (PEXTENDED_IO_STACK_LOCATION)IrpSp;
        ASSERT(IrpSp);

        OutputSize = EIrpSp->Parameters.FileSystemControl.OutputBufferLength;

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        /* This request is not allowed on the main device object */
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;

        /* check Fcb is valid or not */
        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Mcb->Flags, MCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        if (OutputSize < sizeof(LARGE_INTEGER)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite (
                    &Fcb->MainResource, Ext2CanIWait())) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        FcbResourceAcquired = TRUE;

        FileAreaOffset = (PLARGE_INTEGER) Ext2GetUserBuffer(Irp);

        /* probe user buffer */

        _SEH2_TRY {
            if (Irp->RequestorMode != KernelMode) {
                ProbeForWrite(FileAreaOffset, OutputSize, sizeof(UCHAR));
            }

        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {

            Status = STATUS_INVALID_USER_BUFFER;
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        DEBUG(DL_DBG, ("Ext2GetRetrievalPointerBase: FileAreaOffset is 0.\n"));

        FileAreaOffset->QuadPart = 0; // sector offset to the first allocatable unit on the filesystem

        Irp->IoStatus.Information = sizeof(LARGE_INTEGER);

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (!_SEH2_AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2InspectReparseData(
    IN PREPARSE_DATA_BUFFER RDB,
    IN ULONG InputBufferLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!RDB) {
        Status = STATUS_INVALID_PARAMETER;
        goto out;
    }

    if (InputBufferLength < sizeof(REPARSE_DATA_BUFFER)) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto out;
    }

    if (InputBufferLength < RDB->ReparseDataLength) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto out;
    }

    if (RDB->ReparseTag != IO_REPARSE_TAG_SYMLINK) {
        Status = STATUS_NOT_IMPLEMENTED;
        goto out;
    }

    if ((PUCHAR)RDB->SymbolicLinkReparseBuffer.PathBuffer
          + RDB->SymbolicLinkReparseBuffer.SubstituteNameOffset
          + RDB->SymbolicLinkReparseBuffer.SubstituteNameLength
        > (PUCHAR)RDB + InputBufferLength ) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto out;
    }

    if ((PUCHAR)RDB->SymbolicLinkReparseBuffer.PathBuffer
          + RDB->SymbolicLinkReparseBuffer.PrintNameOffset
          + RDB->SymbolicLinkReparseBuffer.PrintNameLength
        > (PUCHAR)RDB + InputBufferLength) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto out;
    }

    if (RDB->SymbolicLinkReparseBuffer.Flags != SYMLINK_FLAG_RELATIVE) {
        Status = STATUS_NOT_IMPLEMENTED;
        goto out;
    }

out:
    return Status;
}

VOID
Ext2InitializeReparseData(IN PREPARSE_DATA_BUFFER RDB, USHORT PathBufferLength)
{
    ASSERT(FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.SubstituteNameOffset) ==
           REPARSE_DATA_BUFFER_HEADER_SIZE);
    RDB->ReparseTag = IO_REPARSE_TAG_SYMLINK;
    RDB->ReparseDataLength = FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) -
                             FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) +
                             PathBufferLength * sizeof(WCHAR);
    RDB->Reserved = 0;
    RDB->SymbolicLinkReparseBuffer.SubstituteNameOffset = PathBufferLength;
    RDB->SymbolicLinkReparseBuffer.SubstituteNameLength = PathBufferLength;
    RDB->SymbolicLinkReparseBuffer.PrintNameOffset = 0;
    RDB->SymbolicLinkReparseBuffer.PrintNameLength = PathBufferLength;
    RDB->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;
    RtlZeroMemory(&RDB->SymbolicLinkReparseBuffer.PathBuffer, PathBufferLength * 2);
}

NTSTATUS
Ext2ReadSymlink (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN PVOID                Buffer,
    IN ULONG                Size,
    OUT PULONG              BytesRead
    )
{
    return Ext2ReadInode (  IrpContext,
                            Vcb,
                            Mcb,
                            0,
                            Buffer,
                            Size,
                            FALSE,
                            BytesRead);
}



NTSTATUS
Ext2GetReparsePoint (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                        Irp = NULL;
    PIO_STACK_LOCATION          IrpSp;
    PEXTENDED_IO_STACK_LOCATION EIrpSp;

    PDEVICE_OBJECT      DeviceObject;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_CCB           Ccb = NULL;
    PEXT2_MCB           Mcb = NULL;

    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    BOOLEAN             MainResourceAcquired = FALSE;

    PVOID               OutputBuffer;
    ULONG               OutputBufferLength;
    ULONG               BytesRead = 0;

    PREPARSE_DATA_BUFFER RDB;

    UNICODE_STRING  UniName;
    OEM_STRING      OemName;

    PCHAR           OemNameBuffer = NULL;
    int             OemNameLength = 0, i;

    Ccb = IrpContext->Ccb;
    ASSERT(Ccb != NULL);
    ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
           (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
    DeviceObject = IrpContext->DeviceObject;
    Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
    Mcb = IrpContext->Fcb->Mcb;
    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    _SEH2_TRY {

        if (!Mcb || !IsInodeSymLink(&Mcb->Inode) ||
            !IsFlagOn(Ccb->Flags, CCB_OPEN_REPARSE_POINT)) {
            Status = STATUS_NOT_A_REPARSE_POINT;
            _SEH2_LEAVE;
        }

        OutputBuffer  = (PVOID)Irp->AssociatedIrp.SystemBuffer;
        OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        RDB = (PREPARSE_DATA_BUFFER)OutputBuffer;
        if (!RDB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        if (OutputBufferLength < sizeof(REPARSE_DATA_BUFFER)) {
            Status = STATUS_BUFFER_OVERFLOW;
            _SEH2_LEAVE;
        }

        OemNameLength = (ULONG)Mcb->Inode.i_size;
        if (OemNameLength > USHRT_MAX) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        OemName.Length = (USHORT)OemNameLength;
        OemName.MaximumLength = OemNameLength + 1;
        OemNameBuffer = OemName.Buffer = Ext2AllocatePool(NonPagedPool,
                                          OemName.MaximumLength,
                                          'NL2E');
        if (!OemNameBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        Status = Ext2ReadSymlink(IrpContext,
                                 Vcb,
                                 Mcb,
                                 OemNameBuffer,
                                 OemNameLength,
                                 &BytesRead
                                );
        OemName.Buffer[OemName.Length] = '\0';
        for (i = 0;i < OemName.Length;i++) {
            if (OemName.Buffer[i] == '/') {
                OemName.Buffer[i] = '\\';
            }
        }

        if (OutputBufferLength - FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) > USHRT_MAX) {
            UniName.Length = USHRT_MAX;
        } else {
            UniName.Length = (USHORT)OutputBufferLength - FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer);
        }
        UniName.MaximumLength = UniName.Length;
        UniName.Length = (USHORT)Ext2OEMToUnicodeSize(Vcb, &OemName);
        Irp->IoStatus.Information = FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + 2 * UniName.Length;
        if (UniName.MaximumLength < 2*UniName.Length) {
            Status = STATUS_BUFFER_TOO_SMALL;
            _SEH2_LEAVE;
        }

        Ext2InitializeReparseData(RDB, UniName.Length);
        UniName.Buffer = RDB->SymbolicLinkReparseBuffer.PathBuffer;
        /*
            (PWCHAR)((PUCHAR)&
             + RDB->SymbolicLinkReparseBuffer.SubstituteNameOffset);
         */
        Ext2OEMToUnicode(Vcb, &UniName, &OemName);
        RtlMoveMemory( (PUCHAR)RDB->SymbolicLinkReparseBuffer.PathBuffer +
                               RDB->SymbolicLinkReparseBuffer.SubstituteNameOffset,
                       UniName.Buffer, UniName.Length);

        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (OemNameBuffer) {
            Ext2FreePool(OemNameBuffer, 'NL2E');
        }

        if (!_SEH2_AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2WriteSymlink (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN PVOID                Buffer,
    IN ULONG                Size,
    OUT PULONG              BytesWritten
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR   Data = (PUCHAR)(&Mcb->Inode.i_block[0]);

    if (Size >= EXT2_LINKLEN_IN_INODE) {

        /* initialize inode i_block[] */
        if (0 == Mcb->Inode.i_blocks) {
            memset(Data, 0, EXT2_LINKLEN_IN_INODE);
            ClearFlag(Mcb->Inode.i_flags, EXT4_EXTENTS_FL);
            Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);
        }

        Status = Ext2WriteInode(IrpContext, Vcb, Mcb,
                                0, Buffer, Size,
                                FALSE, BytesWritten);
        if (!NT_SUCCESS(Status)) {
            goto out;
        }

    } else {

        /* free inode blocks before writing in line */
        if (Mcb->Inode.i_blocks) {
            LARGE_INTEGER Zero = {0, 0};
            Ext2TruncateFile(IrpContext, Vcb, Mcb, &Zero);
        }

        ClearFlag(Mcb->Inode.i_flags, EXT4_EXTENTS_FL);
        memset(Data, 0, EXT2_LINKLEN_IN_INODE);
        RtlCopyMemory(Data, Buffer, Size);
    }

    Mcb->Inode.i_size = Size;
    Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    if (BytesWritten) {
        *BytesWritten = Size;
    }

out:
    return Status;
}

NTSTATUS
Ext2SetReparsePoint (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IrpSp;

    PDEVICE_OBJECT      DeviceObject;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_CCB           Ccb = NULL;
    PEXT2_MCB           Mcb = NULL;

    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PVOID               InputBuffer;
    ULONG               InputBufferLength;
    ULONG               BytesWritten = 0;

    PEXT2_FCB           ParentDcb = NULL;   /* Dcb of it's current parent */
    PEXT2_MCB           ParentMcb = NULL;

    PREPARSE_DATA_BUFFER RDB;

    UNICODE_STRING      UniName;
    OEM_STRING          OemName;

    PCHAR               OemNameBuffer = NULL;
    int                 OemNameLength = 0, i;

    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             FcbLockAcquired = FALSE;

    _SEH2_TRY {

        Ccb = IrpContext->Ccb;
        ASSERT(Ccb != NULL);
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        DeviceObject = IrpContext->DeviceObject;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        Fcb = IrpContext->Fcb;
        Mcb = Fcb->Mcb;
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        ExAcquireResourceExclusiveLite(&Vcb->FcbLock, TRUE);
        FcbLockAcquired = TRUE;

        ParentMcb = Mcb->Parent;
        ParentDcb = ParentMcb->Fcb;
        if (ParentDcb == NULL) {
            ParentDcb = Ext2AllocateFcb(Vcb, ParentMcb);
        }
        if (ParentDcb) {
            Ext2ReferXcb(&ParentDcb->ReferenceCount);
        }

        if (!Mcb)
            _SEH2_LEAVE;

        if (FcbLockAcquired) {
            ExReleaseResourceLite(&Vcb->FcbLock);
            FcbLockAcquired = FALSE;
        }

        if (!ExAcquireResourceSharedLite(
                    &Fcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        MainResourceAcquired = TRUE;

        InputBuffer  = Irp->AssociatedIrp.SystemBuffer;
        InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

        RDB = (PREPARSE_DATA_BUFFER)InputBuffer;
        Status = Ext2InspectReparseData(RDB, InputBufferLength);
        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        UniName.Length = RDB->SymbolicLinkReparseBuffer.SubstituteNameLength;
        UniName.MaximumLength = UniName.Length;
        UniName.Buffer =
            (PWCHAR)((PUCHAR)&RDB->SymbolicLinkReparseBuffer.PathBuffer
             + RDB->SymbolicLinkReparseBuffer.SubstituteNameOffset);

        OemNameLength = Ext2UnicodeToOEMSize(Vcb, &UniName);
        if (OemNameLength > USHRT_MAX) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        OemName.Length = (USHORT)OemNameLength;
        OemName.MaximumLength = OemNameLength + 1;
        OemNameBuffer = OemName.Buffer = Ext2AllocatePool(PagedPool,
                                          OemName.MaximumLength,
                                          'NL2E');
        if (!OemNameBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        Ext2UnicodeToOEM(Vcb, &OemName, &UniName);
        OemName.Buffer[OemName.Length] = '\0';
        for (i = 0;i < OemName.Length;i++) {
            if (OemName.Buffer[i] == '\\') {
                OemName.Buffer[i] = '/';
            }
        }

        /* free all data blocks of the inode (to be set as symlink) */
        {
            LARGE_INTEGER zero = {0};
            Status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &zero);
        }

        /* decrease dir count of group desc and vcb stat */
        if (S_ISDIR(Mcb->Inode.i_mode)) {

            ULONG group = (Mcb->Inode.i_ino - 1) / INODES_PER_GROUP;
            Ext2UpdateGroupDirStat(IrpContext, Vcb, group);

            /* drop extra reference for dir inode */
            ext3_dec_count(&Mcb->Inode);
        }

        /* overwrite inode mode as type SYMLINK */
        Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);
        SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_REPARSE_POINT);

        Status = Ext2WriteSymlink(IrpContext, Vcb, Mcb, OemNameBuffer,
                                  OemNameLength, &BytesWritten);
        if (NT_SUCCESS(Status)) {
            Ext2SetFileType(IrpContext, Vcb, ParentDcb, Mcb,
                            S_IFLNK | S_IRWXUGO);
        }

    } _SEH2_FINALLY {

        if (FcbLockAcquired) {
            ExReleaseResourceLite(&Vcb->FcbLock);
            FcbLockAcquired = FALSE;
        }

        if (MainResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (OemNameBuffer) {
            Ext2FreePool(OemNameBuffer, 'NL2E');
        }

        if (NT_SUCCESS(Status)) {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                FILE_NOTIFY_CHANGE_ATTRIBUTES,
                FILE_ACTION_MODIFIED );
        }

        if (!_SEH2_AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }

        if (ParentDcb) {
            Ext2ReleaseFcb(ParentDcb);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2TruncateSymlink(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    ULONG             Size
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR   data = (PUCHAR)&Mcb->Inode.i_block;
    ULONG    len = (ULONG)Mcb->Inode.i_size;
    LARGE_INTEGER NewSize;

    if (len < EXT2_LINKLEN_IN_INODE && !Mcb->Inode.i_blocks) {

        RtlZeroMemory(data + Size, EXT2_LINKLEN_IN_INODE - Size);
        Mcb->Inode.i_size = Size;
        Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    } else {
        NewSize.QuadPart = Size;
        status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &NewSize);
        if (!NT_SUCCESS(status)) {
            goto out;
        }
    }

out:
    return status;
}


/* FIXME: We can only handle one reparse point right now. */
NTSTATUS
Ext2DeleteReparsePoint (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                        Irp = NULL;

    PDEVICE_OBJECT      DeviceObject;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_CCB           Ccb = NULL;
    PEXT2_MCB           Mcb = NULL;

    PEXT2_FCB           ParentDcb = NULL;   /* Dcb of it's current parent */
    PEXT2_MCB           ParentMcb = NULL;

    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    BOOLEAN             FcbLockAcquired = FALSE;
    BOOLEAN             MainResourceAcquired = FALSE;


    _SEH2_TRY {

        Ccb = IrpContext->Ccb;
        ASSERT(Ccb != NULL);
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        DeviceObject = IrpContext->DeviceObject;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        Mcb = IrpContext->Fcb->Mcb;
        Irp = IrpContext->Irp;

        ExAcquireResourceExclusiveLite(&Vcb->FcbLock, TRUE);
        FcbLockAcquired = TRUE;

        ParentMcb = Mcb->Parent;
        ParentDcb = ParentMcb->Fcb;
        if (ParentDcb == NULL) {
            ParentDcb = Ext2AllocateFcb(Vcb, ParentMcb);
        }
        if (ParentDcb) {
            Ext2ReferXcb(&ParentDcb->ReferenceCount);
        }

        if (!Mcb || !IsInodeSymLink(&Mcb->Inode) ||
            !IsFlagOn(Ccb->Flags, CCB_OPEN_REPARSE_POINT)) {
            Status = STATUS_NOT_A_REPARSE_POINT;
            _SEH2_LEAVE;
        }

        Fcb = Ext2AllocateFcb (Vcb, Mcb);
        if (Fcb) {
            Ext2ReferXcb(&Fcb->ReferenceCount);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        if (FcbLockAcquired) {
            ExReleaseResourceLite(&Vcb->FcbLock);
            FcbLockAcquired = FALSE;
        }

        if (!ExAcquireResourceSharedLite(
                    &Fcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        MainResourceAcquired = TRUE;

        Status = Ext2TruncateSymlink(IrpContext, Vcb, Mcb, 0);
        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        /* inode is to be removed */
        SetFlag(Ccb->Flags, CCB_DELETE_ON_CLOSE);

    } _SEH2_FINALLY {

        if (FcbLockAcquired) {
            ExReleaseResourceLite(&Vcb->FcbLock);
        }

        if (MainResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (NT_SUCCESS(Status)) {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                FILE_NOTIFY_CHANGE_ATTRIBUTES,
                FILE_ACTION_MODIFIED );

        }

        if (!_SEH2_AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }

        if (ParentDcb) {
            Ext2ReleaseFcb(ParentDcb);
        }

        if (Fcb) {
            Ext2ReleaseFcb(Fcb);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2UserFsRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;
    ULONG               FsControlCode;
    NTSTATUS            Status;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    Irp = IrpContext->Irp;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

#ifndef _GNU_NTIFS_
    FsControlCode =
        IoStackLocation->Parameters.FileSystemControl.FsControlCode;
#else
    FsControlCode = ((PEXTENDED_IO_STACK_LOCATION)
                     IoStackLocation)->Parameters.FileSystemControl.FsControlCode;
#endif

    switch (FsControlCode) {

    case FSCTL_GET_REPARSE_POINT:
        Status = Ext2GetReparsePoint(IrpContext);
        break;

    case FSCTL_SET_REPARSE_POINT:
        Status = Ext2SetReparsePoint(IrpContext);
        break;

    case FSCTL_DELETE_REPARSE_POINT:
        Status = Ext2DeleteReparsePoint(IrpContext);
        break;

    case FSCTL_LOCK_VOLUME:
        Status = Ext2LockVolume(IrpContext);
        break;

    case FSCTL_UNLOCK_VOLUME:
        Status = Ext2UnlockVolume(IrpContext);
        break;

    case FSCTL_DISMOUNT_VOLUME:
        Status = Ext2DismountVolume(IrpContext);
        break;

    case FSCTL_IS_VOLUME_MOUNTED:
        Status = Ext2IsVolumeMounted(IrpContext);
        break;

    case FSCTL_INVALIDATE_VOLUMES:
        Status = Ext2InvalidateVolumes(IrpContext);
        break;

#if (_WIN32_WINNT >= 0x0500)
    case FSCTL_ALLOW_EXTENDED_DASD_IO:
        Status = Ext2AllowExtendedDasdIo(IrpContext);
        break;
#endif //(_WIN32_WINNT >= 0x0500)

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        Status = Ext2OplockRequest(IrpContext);
        break;

    case FSCTL_IS_VOLUME_DIRTY:
        Status = Ext2IsVolumeDirty(IrpContext);
        break;

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        Status = Ext2QueryRetrievalPointers(IrpContext);
        break;

    case FSCTL_GET_RETRIEVAL_POINTERS:
        Status = Ext2GetRetrievalPointers(IrpContext);
        break;

    case FSCTL_GET_RETRIEVAL_POINTER_BASE:
        Status = Ext2GetRetrievalPointerBase(IrpContext);
        break;

    default:

        DEBUG(DL_INF, ( "Ext2UserFsRequest: Invalid User Request: %xh.\n", FsControlCode));
        Status = STATUS_INVALID_DEVICE_REQUEST;

        Ext2CompleteIrpContext(IrpContext,  Status);
    }

    return Status;
}

BOOLEAN
Ext2IsMediaWriteProtected (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PDEVICE_OBJECT TargetDevice
)
{
    PIRP            Irp;
    KEVENT          Event;
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
                                         TargetDevice,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &IoStatus );

    if (Irp == NULL) {
        return FALSE;
    }

    SetFlag(IoGetNextIrpStackLocation(Irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    Status = IoCallDriver(TargetDevice, Irp);

    if (Status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = IoStatus.Status;
    }

    return (BOOLEAN)(Status == STATUS_MEDIA_WRITE_PROTECTED);
}

NTSTATUS
Ext2MountVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT              MainDeviceObject;
    BOOLEAN                     GlobalDataResourceAcquired = FALSE;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IoStackLocation;
    PDEVICE_OBJECT              TargetDeviceObject;
    NTSTATUS                    Status = STATUS_UNRECOGNIZED_VOLUME;
    PDEVICE_OBJECT              VolumeDeviceObject = NULL;
    PEXT2_VCB                   Vcb = NULL, OldVcb = NULL;
    PVPB                        OldVpb = NULL, Vpb = NULL;
    PEXT2_SUPER_BLOCK           Ext2Sb = NULL;
    ULONG                       dwBytes;
    DISK_GEOMETRY               DiskGeometry;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        MainDeviceObject = IrpContext->DeviceObject;

        //
        //  Make sure we can wait.
        //

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

        //
        // This request is only allowed on the main device object
        //
        if (!IsExt2FsDevice(MainDeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        if (IsFlagOn(Ext2Global->Flags, EXT2_UNLOAD_PENDING)) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
        }

#if 0
        if (IrpContext->RealDevice->Size >= sizeof(ULONG) + sizeof(DEVICE_OBJECT) &&
                *((PULONG)IrpContext->RealDevice->DeviceExtension) == 'DSSA') {
        } else {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
        }
#endif

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        TargetDeviceObject =
            IoStackLocation->Parameters.MountVolume.DeviceObject;

        dwBytes = sizeof(DISK_GEOMETRY);
        Status = Ext2DiskIoControl(
                     TargetDeviceObject,
                     IOCTL_DISK_GET_DRIVE_GEOMETRY,
                     NULL,
                     0,
                     &DiskGeometry,
                     &dwBytes );

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        Status = IoCreateDevice(
                     MainDeviceObject->DriverObject,
                     sizeof(EXT2_VCB),
                     NULL,
                     FILE_DEVICE_DISK_FILE_SYSTEM,
                     0,
                     FALSE,
                     &VolumeDeviceObject );

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }
        INC_MEM_COUNT(PS_VCB, VolumeDeviceObject, sizeof(EXT2_VCB));

#ifdef _PNP_POWER_
        /* don't care about power management requests */
        VolumeDeviceObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif

        VolumeDeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);
        ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);

/*
        These are for buffer-address alignment requirements.
        Never do this check, unless you want fail user requests :)

        if (TargetDeviceObject->AlignmentRequirement >
                VolumeDeviceObject->AlignmentRequirement) {

            VolumeDeviceObject->AlignmentRequirement =
                TargetDeviceObject->AlignmentRequirement;
        }

        if (DiskGeometry.BytesPerSector - 1 >
                VolumeDeviceObject->AlignmentRequirement) {
            VolumeDeviceObject->AlignmentRequirement =
                DiskGeometry.BytesPerSector - 1;
            TargetDeviceObject->AlignmentRequirement =
                DiskGeometry.BytesPerSector - 1;
        }
*/
        (IoStackLocation->Parameters.MountVolume.Vpb)->DeviceObject =
            VolumeDeviceObject;
        Vpb = IoStackLocation->Parameters.MountVolume.Vpb;

        Vcb = (PEXT2_VCB) VolumeDeviceObject->DeviceExtension;

        RtlZeroMemory(Vcb, sizeof(EXT2_VCB));
        Vcb->Identifier.Type = EXT2VCB;
        Vcb->Identifier.Size = sizeof(EXT2_VCB);
        Vcb->TargetDeviceObject = TargetDeviceObject;
        Vcb->DiskGeometry = DiskGeometry;
        InitializeListHead(&Vcb->Next);

        Status = Ext2LoadSuper(Vcb, FALSE, &Ext2Sb);
        if (!NT_SUCCESS(Status)) {
            Vcb = NULL;
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
        }
        ASSERT (NULL != Ext2Sb);

        /* check Linux Ext2/Ext3 volume magic */
        if (Ext2Sb->s_magic == EXT2_SUPER_MAGIC) {
            DEBUG(DL_INF, ( "Volume of ext2 file system is found.\n"));
        } else  {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            Vcb = NULL;
            _SEH2_LEAVE;
        }

        DEBUG(DL_DBG, ("Ext2MountVolume: DevObject=%p Vcb=%p\n", VolumeDeviceObject, Vcb));

        /* initialize Vcb structure */
        Status = Ext2InitializeVcb( IrpContext, Vcb, Ext2Sb,
                                    TargetDeviceObject,
                                    VolumeDeviceObject, Vpb);

        if (NT_SUCCESS(Status))  {

            PLIST_ENTRY List;

            ExAcquireResourceExclusiveLite(&(Ext2Global->Resource), TRUE);
            GlobalDataResourceAcquired = TRUE;

            for (List = Ext2Global->VcbList.Flink;
                    List != &Ext2Global->VcbList;
                    List = List->Flink) {

                OldVcb = CONTAINING_RECORD(List, EXT2_VCB, Next);
                OldVpb = OldVcb->Vpb;

                /* in case we are already in the queue, should not happen */
                if (OldVpb == Vpb) {
                    continue;
                }

                if ( (OldVpb->SerialNumber == Vpb->SerialNumber) &&
                        (!IsMounted(OldVcb)) && (IsFlagOn(OldVcb->Flags, VCB_NEW_VPB)) &&
                        (OldVpb->RealDevice == TargetDeviceObject) &&
                        (OldVpb->VolumeLabelLength == Vpb->VolumeLabelLength) &&
                        (RtlEqualMemory(&OldVpb->VolumeLabel[0],
                                        &Vpb->VolumeLabel[0],
                                        Vpb->VolumeLabelLength)) &&
                        (RtlEqualMemory(&OldVcb->SuperBlock->s_uuid[0],
                                        &Vcb->SuperBlock->s_uuid[0], 16)) ) {
                    ClearFlag(OldVcb->Flags, VCB_MOUNTED);
                }
            }

            SetLongFlag(Vcb->Flags, VCB_MOUNTED);
            SetFlag(Vcb->Vpb->Flags, VPB_MOUNTED);
            Ext2InsertVcb(Vcb);
            Vcb = NULL;
            Vpb = NULL;
            ObDereferenceObject(TargetDeviceObject);

        } else {

            Vcb = NULL;
        }

    } _SEH2_FINALLY {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!NT_SUCCESS(Status)) {

            if (!NT_SUCCESS(Status)) {
                if ( Vpb != NULL ) {
                    Vpb->DeviceObject = NULL;
                }
            }

            if (Vcb) {
                Ext2DestroyVcb(Vcb);
            } else {
                if (Ext2Sb) {
                    Ext2FreePool(Ext2Sb, EXT2_SB_MAGIC);
                }
                if (VolumeDeviceObject) {
                    IoDeleteDevice(VolumeDeviceObject);
                    DEC_MEM_COUNT(PS_VCB, VolumeDeviceObject, sizeof(EXT2_VCB));
                }
            }
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    } _SEH2_END;

    return Status;
}

VOID
Ext2VerifyVcb (IN PEXT2_IRP_CONTEXT IrpContext,
               IN PEXT2_VCB         Vcb )
{
    NTSTATUS                Status = STATUS_SUCCESS;

    BOOLEAN                 bVerify = FALSE;
    ULONG                   ChangeCount = 0;
    ULONG                   dwBytes;

    PIRP                    Irp;
    PEXTENDED_IO_STACK_LOCATION      IrpSp;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        Irp = IrpContext->Irp;
        IrpSp = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);

        bVerify = IsFlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

        if ( (IsFlagOn(Vcb->Flags, VCB_REMOVABLE_MEDIA) ||
                IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) && !bVerify ) {

            dwBytes = sizeof(ULONG);
            Status = Ext2DiskIoControl(
                         Vcb->TargetDeviceObject,
                         IOCTL_DISK_CHECK_VERIFY,
                         NULL,
                         0,
                         &ChangeCount,
                         &dwBytes );

            if ( STATUS_VERIFY_REQUIRED == Status ||
                    STATUS_DEVICE_NOT_READY == Status ||
                    STATUS_NO_MEDIA_IN_DEVICE == Status ||
                    (NT_SUCCESS(Status) &&
                     (ChangeCount != Vcb->ChangeCount))) {

                KIRQL Irql;

                IoAcquireVpbSpinLock(&Irql);
                if (Vcb->Vpb == Vcb->Vpb->RealDevice->Vpb) {
                    SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);
                }
                IoReleaseVpbSpinLock(Irql);

            } else {

                if (!NT_SUCCESS(Status)) {
                    Ext2NormalizeAndRaiseStatus(IrpContext, Status);
                }
            }
        }

        if ( IsFlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {
            IoSetHardErrorOrVerifyDevice( Irp, Vcb->Vpb->RealDevice );
            Ext2NormalizeAndRaiseStatus ( IrpContext,
                                          STATUS_VERIFY_REQUIRED );
        }

        if (IsMounted(Vcb)) {

            if ( (IrpContext->MajorFunction == IRP_MJ_WRITE) ||
                    (IrpContext->MajorFunction == IRP_MJ_SET_INFORMATION) ||
                    (IrpContext->MajorFunction == IRP_MJ_SET_EA) ||
                    (IrpContext->MajorFunction == IRP_MJ_FLUSH_BUFFERS) ||
                    (IrpContext->MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION) ||
                    (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                     IrpContext->MinorFunction == IRP_MN_USER_FS_REQUEST &&
                     IrpSp->Parameters.FileSystemControl.FsControlCode ==
                     FSCTL_MARK_VOLUME_DIRTY)) {

                if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {

                    KIRQL Irql;

                    IoAcquireVpbSpinLock(&Irql);
                    if (Vcb->Vpb == Vcb->Vpb->RealDevice->Vpb) {
                        SetFlag (Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);
                    }
                    IoReleaseVpbSpinLock(Irql);

                    IoSetHardErrorOrVerifyDevice( Irp, Vcb->Vpb->RealDevice );

                    Ext2RaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                }
            }
        }

    } _SEH2_FINALLY {

    } _SEH2_END;

}


NTSTATUS
Ext2VerifyVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_SUPER_BLOCK       ext2_sb = NULL;
    PEXT2_VCB               Vcb = NULL;
    BOOLEAN                 VcbResourceAcquired = FALSE;
    PIRP                    Irp;
    ULONG                   ChangeCount = 0;
    ULONG                   dwBytes;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        VcbResourceAcquired =
            ExAcquireResourceExclusiveLite(
                &Vcb->MainResource,
                TRUE );

        if (!FlagOn(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME)) {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (!IsMounted(Vcb)) {
            Status = STATUS_WRONG_VOLUME;
            _SEH2_LEAVE;
        }

        dwBytes = sizeof(ULONG);
        Status = Ext2DiskIoControl(
                     Vcb->TargetDeviceObject,
                     IOCTL_DISK_CHECK_VERIFY,
                     NULL,
                     0,
                     &ChangeCount,
                     &dwBytes );


        if (!NT_SUCCESS(Status)) {
            Status = STATUS_WRONG_VOLUME;
            _SEH2_LEAVE;
        } else {
            Vcb->ChangeCount = ChangeCount;
        }

        Irp = IrpContext->Irp;

        Status = Ext2LoadSuper(Vcb, TRUE, &ext2_sb);

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        ASSERT(NULL != ext2_sb);
        if ((ext2_sb->s_magic == EXT2_SUPER_MAGIC) &&
                (memcmp(ext2_sb->s_uuid, SUPER_BLOCK->s_uuid, 16) == 0) &&
                (memcmp(ext2_sb->s_volume_name, SUPER_BLOCK->s_volume_name, 16) ==0)) {

            ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);

            if (Ext2IsMediaWriteProtected(IrpContext, Vcb->TargetDeviceObject)) {
                SetLongFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
            } else {
                ClearLongFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
            }

            DEBUG(DL_INF, ( "Ext2VerifyVolume: Volume verify succeeded.\n"));

        } else {

            Status = STATUS_WRONG_VOLUME;
            Ext2PurgeVolume(Vcb, FALSE);

            SetLongFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);
            ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);

            DEBUG(DL_INF, ( "Ext2VerifyVolume: Volume verify failed.\n"));
        }

    } _SEH2_FINALLY {

        if (ext2_sb)
            Ext2FreePool(ext2_sb, EXT2_SB_MAGIC);

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2IsVolumeMounted (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT      DeviceObject;
    PEXT2_VCB           Vcb = 0;
    NTSTATUS            Status = STATUS_SUCCESS;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));


    DeviceObject = IrpContext->DeviceObject;

    Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

    ASSERT(IsMounted(Vcb));

    Ext2VerifyVcb (IrpContext, Vcb);

    Ext2CompleteIrpContext(IrpContext,  Status);

    return Status;
}


NTSTATUS
Ext2DismountVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB       Vcb = NULL;
    BOOLEAN         VcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );

        VcbResourceAcquired = TRUE;

        if ( IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
            Status = STATUS_VOLUME_DISMOUNTED;
            _SEH2_LEAVE;
        }

        Ext2FlushFiles(IrpContext, Vcb, FALSE);
        Ext2FlushVolume(IrpContext, Vcb, FALSE);

        ExReleaseResourceLite(&Vcb->MainResource);
        VcbResourceAcquired = FALSE;

        Ext2PurgeVolume(Vcb, TRUE);
        Ext2CheckDismount(IrpContext, Vcb, TRUE);

        DEBUG(DL_INF, ( "Ext2Dismount: Volume dismount pending.\n"));
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    } _SEH2_END;

    return Status;
}

BOOLEAN
Ext2CheckDismount (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN BOOLEAN           bForce   )
{
    KIRQL   Irql;
    PVPB    Vpb = Vcb->Vpb, NewVpb = NULL;
    BOOLEAN bDeleted = FALSE, bTearDown = FALSE;
    ULONG   UnCleanCount = 0;

    NewVpb = ExAllocatePoolWithTag(NonPagedPool, VPB_SIZE, TAG_VPB);
    if (NewVpb == NULL) {
        DEBUG(DL_ERR, ( "Ex2CheckDismount: failed to allocate NewVpb.\n"));
        return FALSE;
    }
    DEBUG(DL_DBG, ("Ext2CheckDismount: NewVpb allocated: %p\n", NewVpb));
    INC_MEM_COUNT(PS_VPB, NewVpb, sizeof(VPB));
    memset(NewVpb, '_', VPB_SIZE);
    RtlZeroMemory(NewVpb, sizeof(VPB));

    ExAcquireResourceExclusiveLite(
        &Ext2Global->Resource, TRUE );

    ExAcquireResourceExclusiveLite(
        &Vcb->MainResource, TRUE );

    if (IrpContext &&
        IrpContext->MajorFunction == IRP_MJ_CREATE &&
        IrpContext->RealDevice == Vcb->RealDevice) {
        UnCleanCount = 2;
    } else {
        UnCleanCount = 1;
    }

    IoAcquireVpbSpinLock (&Irql);

    DEBUG(DL_DBG, ("Ext2CheckDismount: Vpb %p ioctl=%d Device %p\n",
                   Vpb, Vpb->ReferenceCount, Vpb->RealDevice));

    if (Vpb->ReferenceCount <= UnCleanCount) {

        if (!IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {

            ClearFlag(Vpb->Flags, VPB_MOUNTED);
            ClearFlag(Vpb->Flags, VPB_LOCKED);

            if ((Vcb->RealDevice != Vpb->RealDevice) &&
                    (Vcb->RealDevice->Vpb == Vpb)) {
                SetFlag(Vcb->RealDevice->Flags, DO_DEVICE_INITIALIZING);
                SetFlag(Vpb->Flags, VPB_PERSISTENT );
            }

            Ext2RemoveVcb(Vcb);
            SetLongFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);
        }

        if (Vpb->ReferenceCount) {
            bTearDown = TRUE;
        } else {
            bDeleted = TRUE;
            Vpb->DeviceObject = NULL;
        }

        DEBUG(DL_DBG, ("Ext2CheckDismount: Vpb: %p bDeleted=%d bTearDown=%d\n",
                        Vpb, bDeleted, bTearDown));


    } else if (bForce) {

        DEBUG(DL_DBG, ( "Ext2CheckDismount: New/Old Vpb %p/%p Realdevice = %p\n",
                        NewVpb, Vcb->Vpb, Vpb->RealDevice));

        /* keep vpb president and later we'll free it */
        SetFlag(Vpb->Flags, VPB_PERSISTENT);

        Vcb->Vpb2 = Vcb->Vpb;
        NewVpb->Type = IO_TYPE_VPB;
        NewVpb->Size = sizeof(VPB);
        NewVpb->Flags = Vpb->Flags & VPB_REMOVE_PENDING;
        NewVpb->RealDevice = Vpb->RealDevice;
        NewVpb->RealDevice->Vpb = NewVpb;
        NewVpb = NULL;
        ClearFlag(Vpb->Flags, VPB_MOUNTED);
        SetLongFlag(Vcb->Flags, VCB_NEW_VPB);
        ClearLongFlag(Vcb->Flags, VCB_MOUNTED);
    }

    IoReleaseVpbSpinLock(Irql);

    ExReleaseResourceLite(&Vcb->MainResource);
    ExReleaseResourceLite(&Ext2Global->Resource);

    if (bTearDown) {
        DEBUG(DL_DBG, ( "Ext2CheckDismount: Tearing vcb %p ...\n", Vcb));
        Ext2TearDownStream(Vcb);
    }

    if (bDeleted) {
        DEBUG(DL_DBG, ( "Ext2CheckDismount: Deleting vcb %p ...\n", Vcb));
        Ext2DestroyVcb(Vcb);
    }

    if (NewVpb != NULL) {
        DEBUG(DL_DBG, ( "Ext2CheckDismount: freeing new Vpb %p\n", NewVpb));
        ExFreePoolWithTag(NewVpb, TAG_VPB);
        DEC_MEM_COUNT(PS_VPB, NewVpb, sizeof(VPB));
    }

    return bDeleted;
}

NTSTATUS
Ext2PurgeVolume (IN PEXT2_VCB Vcb,
                 IN BOOLEAN  FlushBeforePurge )
{
    PEXT2_FCB       Fcb;
    LIST_ENTRY      List, *Next;

    BOOLEAN         VcbResourceAcquired = FALSE;
    BOOLEAN         FcbResourceAcquired = FALSE;
    BOOLEAN         gdResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
        VcbResourceAcquired = TRUE;

        if (IsVcbReadOnly(Vcb)) {
            FlushBeforePurge = FALSE;
        }

        InitializeListHead(&List);

        ExAcquireResourceExclusiveLite(&Vcb->FcbLock, TRUE);
        FcbResourceAcquired = TRUE;

        while (!IsListEmpty(&Vcb->FcbList)) {

            Next = RemoveHeadList(&Vcb->FcbList);
            Fcb = CONTAINING_RECORD(Next, EXT2_FCB, Next);

            DEBUG(DL_INF, ( "Ext2PurgeVolume: %wZ refercount=%xh\n",
                            &Fcb->Mcb->FullName, Fcb->ReferenceCount));
            InsertTailList(&List, &Fcb->Next);
        }

        while (!IsListEmpty(&List)) {

            Next = RemoveHeadList(&List);
            Fcb = CONTAINING_RECORD(Next, EXT2_FCB, Next);

            if (ExAcquireResourceExclusiveLite(
                        &Fcb->MainResource,
                        TRUE )) {

                Ext2PurgeFile(Fcb, FlushBeforePurge);

                if (Fcb->ReferenceCount <= 1) {
                    Fcb->TsDrop.QuadPart = 0;
                    InsertHeadList(&Vcb->FcbList, &Fcb->Next);
                } else {
                    InsertTailList(&Vcb->FcbList, &Fcb->Next);
                }
                ExReleaseResourceLite(&Fcb->MainResource);
            }
        }

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->FcbLock);
            FcbResourceAcquired = FALSE;
        }

        /* acquire bd lock to avoid bh creation */
        ExAcquireResourceExclusiveLite(&Vcb->sbi.s_gd_lock, TRUE);
        gdResourceAcquired = TRUE;

        /* discard buffer_headers for group_desc */
        Ext2DropBH(Vcb);

        if (FlushBeforePurge) {
            ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Vcb->PagingIoResource);

            CcFlushCache(&Vcb->SectionObject, NULL, 0, NULL);
        }

        if (Vcb->SectionObject.ImageSectionObject) {
            MmFlushImageSection(&Vcb->SectionObject, MmFlushForWrite);
        }

        if (Vcb->SectionObject.DataSectionObject) {
            CcPurgeCacheSection(&Vcb->SectionObject, NULL, 0, FALSE);
        }

        DEBUG(DL_INF, ( "Ext2PurgeVolume: Volume flushed and purged.\n"));

    } _SEH2_FINALLY {

        if (gdResourceAcquired) {
            ExReleaseResourceLite(&Vcb->sbi.s_gd_lock);
        }

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->FcbLock);
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }
    } _SEH2_END;

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2PurgeFile ( IN PEXT2_FCB Fcb,
                IN BOOLEAN  FlushBeforePurge )
{
    IO_STATUS_BLOCK    IoStatus;

    ASSERT(Fcb != NULL);

    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));


    if (!IsVcbReadOnly(Fcb->Vcb) && FlushBeforePurge) {
        DEBUG(DL_INF, ( "Ext2PurgeFile: CcFlushCache on %wZ.\n",
                        &Fcb->Mcb->FullName));
        ExAcquireSharedStarveExclusive(&Fcb->PagingIoResource, TRUE);
        ExReleaseResourceLite(&Fcb->PagingIoResource);
        CcFlushCache(&Fcb->SectionObject, NULL, 0, &IoStatus);
        ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
    }

    if (Fcb->SectionObject.ImageSectionObject) {
        DEBUG(DL_INF, ( "Ext2PurgeFile: MmFlushImageSection on %wZ.\n",
                        &Fcb->Mcb->FullName));
        MmFlushImageSection(&Fcb->SectionObject, MmFlushForWrite);
    }

    if (Fcb->SectionObject.DataSectionObject) {
        DEBUG(DL_INF, ( "Ext2PurgeFile: CcPurgeCacheSection on %wZ.\n",
                        &Fcb->Mcb->FullName));
        CcPurgeCacheSection(&Fcb->SectionObject, NULL, 0, FALSE);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
Ext2FileSystemControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS    Status;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    switch (IrpContext->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:
        Status = Ext2UserFsRequest(IrpContext);
        break;

    case IRP_MN_MOUNT_VOLUME:
        Status = Ext2MountVolume(IrpContext);
        break;

    case IRP_MN_VERIFY_VOLUME:
        Status = Ext2VerifyVolume(IrpContext);
        break;

    default:

        DEBUG(DL_ERR, ( "Ext2FilsSystemControl: Invalid Device Request.\n"));
        Status = STATUS_INVALID_DEVICE_REQUEST;
        Ext2CompleteIrpContext(IrpContext,  Status);
    }

    return Status;
}

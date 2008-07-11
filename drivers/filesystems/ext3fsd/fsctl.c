/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fsctl.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
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

    for( List = Vcb->FcbList.Flink;
         List != &Vcb->FcbList;
         List = List->Flink )  {

        Fcb = CONTAINING_RECORD(List, EXT2_FCB, Next);

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        DEBUG(DL_INF, ( "Ext2IsHandleCountZero: Inode:%xh File:%S OpenHandleCount=%xh\n",
                  Fcb->Mcb->iNo, Fcb->Mcb->ShortName.Buffer, Fcb->OpenHandleCount));

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

    __try {

        if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            DEBUG(DL_INF, ( "Ext2LockVolume: Volume is already locked.\n"));
            Status = STATUS_ACCESS_DENIED;
            __leave;
        }
        
        if (Vcb->OpenFileHandleCount > (ULONG)(FileObject ? 1 : 0)) {
            DEBUG(DL_INF, ( "Ext2LockVcb: There are still opened files.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            __leave;
        }
        
        if (!Ext2IsHandleCountZero(Vcb)) {
            DEBUG(DL_INF, ( "Ext2LockVcb: Thare are still opened files.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            __leave;
        }
        
        SetLongFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        Ext2SetVpbFlag(Vcb->Vpb, VPB_LOCKED);
        Vcb->LockFile = FileObject;
        
        DEBUG(DL_INF, ( "Ext2LockVcb: Volume locked.\n"));

    } __finally {
        // Nothing
    }
    
    return Status;
}


NTSTATUS
Ext2LockVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT  DeviceObject;
    PEXT2_VCB       Vcb;
    NTSTATUS        Status;
    BOOLEAN VcbResourceAcquired = FALSE;
   
    __try {

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
            __leave;
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

        Status = Ext2LockVcb(Vcb, IrpSp->FileObject);        

    } __finally {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }
        
        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}

NTSTATUS
Ext2UnlockVcb ( IN PEXT2_VCB    Vcb,
                IN PFILE_OBJECT FileObject )
{
    NTSTATUS        Status;

    __try {

        if (FileObject && FileObject->FsContext != Vcb) {
            Status = STATUS_NOT_LOCKED;
            __leave;
        }
       
        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            DEBUG(DL_ERR, ( ": Ext2UnlockVcb: Volume is not locked.\n"));
            Status = STATUS_NOT_LOCKED;
            __leave;
        }

        if (Vcb->LockFile == FileObject) {
            ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
            Ext2ClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
            DEBUG(DL_INF, ( "Ext2UnlockVcb: Volume unlocked.\n"));
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NOT_LOCKED;
        }

    } __finally {
        // Nothing
    }

    return Status;
}

NTSTATUS
Ext2UnlockVolume (
         IN PEXT2_IRP_CONTEXT IrpContext
         )
{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status;
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try {

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
            __leave;
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

    } __finally {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }
        
        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}


NTSTATUS
Ext2InvalidateVolumes ( IN PEXT2_IRP_CONTEXT IrpContext )
{
    NTSTATUS            Status;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;

    PVPB                NewVpb = NULL;
    HANDLE              Handle;
    PLIST_ENTRY         ListEntry;

    ULONG               InputLength = 0;
    PFILE_OBJECT        FileObject;
    PDEVICE_OBJECT      DeviceObject;
    BOOLEAN             GlobalResourceAcquired = FALSE;

    LUID Privilege = {SE_TCB_PRIVILEGE, 0};

    __try {

        Irp   = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (!IsExt2FsDevice(IrpSp->DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        if (!SeSinglePrivilegeCheck(Privilege, Irp->RequestorMode)) {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            __leave;
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
                __leave;
            }
            Handle = (HANDLE) LongToHandle( (*(PUINT32)Irp->AssociatedIrp.SystemBuffer) );
        } else
#endif
         {
            if (InputLength != sizeof(HANDLE)) {
                Status = STATUS_INVALID_PARAMETER;
                __leave;
            }
            Handle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
        }

        Status = ObReferenceObjectByHandle( Handle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            &FileObject,
                                            NULL );

        if (!NT_SUCCESS(Status)) {
            __leave;
        } else {
            ObDereferenceObject(FileObject);
            DeviceObject = FileObject->DeviceObject;
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

                ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);

                if (Vcb->Vpb == DeviceObject->Vpb) {

                    KIRQL irql;
                    IoAcquireVpbSpinLock(&irql);

                    if (IsFlagOn(DeviceObject->Vpb->Flags, VPB_MOUNTED)) {

                        PVPB NewVpb1 = ExAllocatePoolWithTag( NonPagedPool,
                                                             VPB_SIZE,
                                                             TAG_VPB);
                        if (NewVpb1 == NULL) {
                            DEBUG(DL_ERR, ( "Ex2InvalidateVolumes: failed to allocate NewVpb.\n"));
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            IoReleaseVpbSpinLock(irql);
                            ExReleaseResourceLite(&Vcb->MainResource);
                            __leave;
                        }

                        RtlZeroMemory( NewVpb1, sizeof(VPB));
                        NewVpb1->Type = IO_TYPE_VPB;
                        NewVpb1->Size = sizeof( VPB );
                        NewVpb1->RealDevice = DeviceObject;
                        NewVpb1->Flags = FlagOn(DeviceObject->Vpb->Flags, VPB_REMOVE_PENDING);
                        DeviceObject->Vpb = NewVpb1;
                    }

                    IoReleaseVpbSpinLock(irql);

                    DEBUG(DL_INF, ( "Ext2InvalidateVolumes: Ext2PurgeVolume...\n"));
                    Ext2PurgeVolume(Vcb, FALSE);
                }

                ExReleaseResourceLite(&Vcb->MainResource);

                /* dismount the volume */        
                Ext2CheckDismount(IrpContext, Vcb, FALSE);
            }
        }

    } __finally {

        if (GlobalResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    }

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

    __try {

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
            __leave;
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
            __leave;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            __leave;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
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

    } __finally {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!AbnormalTermination()) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    }

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

    __try {

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
            __leave;
        }

        if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {
            status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        *VolumeState = 0;

    } __finally {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  status);
        }
    }

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

    __try {

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
                __leave;
            }

            if (Chain) {
                Ext2JointExtents(Chain, Extent);
            } else {
                Chain = Extent;
            }

            /* allocate extent array */
            PartialRuns = ExAllocatePoolWithTag(
                            NonPagedPool,
                            (Ext2CountExtents(Chain) + 2) * 
                            (2 * sizeof(LARGE_INTEGER)),
                            'RE2E');

            if (PartialRuns == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }
            RtlZeroMemory(  PartialRuns,
                            (Ext2CountExtents(Chain) + 2) * 
                            (2 * sizeof(LARGE_INTEGER)));

            if (MappedRuns) {
                RtlMoveMemory(PartialRuns,
                              MappedRuns,
                              i * 2 * sizeof(LARGE_INTEGER));
                ExFreePoolWithTag(MappedRuns, 'RE2E');
            }
            MappedRuns = PartialRuns;

            /* walk all the Mcb runs in Extent */
            for (;Extent != NULL; Extent = Extent->Next) {
                MappedRuns[i*2 + 0].QuadPart = Vbn + Extent->Offset;
                MappedRuns[i*2 + 1].QuadPart = Extent->Lba;
                i = i+1;
            }

            Vbn = Vbn + Length;
        }

        *pMappedRuns = MappedRuns;

    } __finally {

        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING) {
            if (MappedRuns) {
                ExFreePoolWithTag(MappedRuns, 'RE2E');
            }
            *pMappedRuns = NULL;
        }

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }
    }

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

    __try {

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
            __leave;
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
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            __leave;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }        

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        /* Is requstor in kernel and Fcb a paging file ? */
        if (Irp->RequestorMode != KernelMode ||
            !IsFlagOn(Fcb->Flags, FCB_PAGE_FILE) || 
            InputSize != sizeof(LARGE_INTEGER) ||
            OutputSize != sizeof(PVOID)) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        if (!ExAcquireResourceExclusiveLite (
                &Fcb->MainResource, Ext2CanIWait())) {
            Status = STATUS_PENDING;
            __leave;
        }
        FcbResourceAcquired = TRUE;

        RequestVbn  = EIrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        pMappedRuns = Irp->UserBuffer;

        DbgBreak();

        /* request size beyonds whole file size */
        if (RequestVbn->QuadPart >= Fcb->Header.AllocationSize.QuadPart) {
            Status = STATUS_END_OF_FILE;
            __leave;
        }

        Status = Ext2QueryExtentMappings(
                    IrpContext,
                    Vcb,
                    Fcb,
                    RequestVbn,
                    pMappedRuns
                    );

    } __finally {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (!AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    }

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

    __try {

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
            __leave;
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
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            __leave;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }        

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        if (InputSize  < sizeof(STARTING_VCN_INPUT_BUFFER) ||
            OutputSize < sizeof(RETRIEVAL_POINTERS_BUFFER) ) {
            Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        if (!ExAcquireResourceExclusiveLite (
                &Fcb->MainResource, Ext2CanIWait())) {
            Status = STATUS_PENDING;
            __leave;
        }
        FcbResourceAcquired = TRUE;

        SVIB = (PSTARTING_VCN_INPUT_BUFFER) 
                EIrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        RPSB = (PRETRIEVAL_POINTERS_BUFFER) Ext2GetUserBuffer(Irp);

        /* probe user buffer */

        __try {
            ProbeForRead (SVIB, InputSize,  sizeof(UCHAR));
            ProbeForWrite(RPSB, OutputSize, sizeof(UCHAR));

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        UsedSize = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[0]);

        /* request size beyonds whole file size ? */
        DEBUG(DL_USR, ("Ext2GetRetrievalPointers: Startin from Vbn: %I64xh\n",
                        SVIB->StartingVcn.QuadPart));
        Vbn = (SVIB->StartingVcn.QuadPart << BLOCK_BITS);
        if (Vbn >= Fcb->Header.AllocationSize.QuadPart ) {
            Status = STATUS_END_OF_FILE;
            __leave;
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
                __leave;
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
                    __leave;
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
                   MappedRuns[_i*2 + 1].QuadPart != 0 ){
                DEBUG(DL_MAP, ("Ext2QueryExtentMappings: %wZ %d Vbn = %I64xh Lbn = %I64xh\n",
                                &Fcb->Mcb->FullName, _i,
                                MappedRuns[_i*2 + 0].QuadPart,
                                MappedRuns[_i*2 + 1].QuadPart));
                _i++;
            }

exit_to_get_rps:

            if (MappedRuns) {
                ExFreePoolWithTag(MappedRuns, 'RE2E');
            }
        }
#endif

    } __finally {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }

        if (!AbnormalTermination()) {
            if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    }

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

    __try {

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
            __leave;
        }

        if (IsFlagOn(Ext2Global->Flags, EXT2_UNLOAD_PENDING)) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            __leave;
        }
        
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
            __leave;
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
            __leave;
        }
        INC_MEM_COUNT(PS_VCB, VolumeDeviceObject, sizeof(EXT2_VCB));

        VolumeDeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);
        ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);

        if (TargetDeviceObject->AlignmentRequirement > 
            VolumeDeviceObject->AlignmentRequirement) {

            VolumeDeviceObject->AlignmentRequirement = 
                TargetDeviceObject->AlignmentRequirement;
        }

        (IoStackLocation->Parameters.MountVolume.Vpb)->DeviceObject =
            VolumeDeviceObject;
        Vpb = IoStackLocation->Parameters.MountVolume.Vpb;

        Vcb = (PEXT2_VCB) VolumeDeviceObject->DeviceExtension;

        RtlZeroMemory(Vcb, sizeof(EXT2_VCB));
        Vcb->Identifier.Type = EXT2VCB;
        Vcb->Identifier.Size = sizeof(EXT2_VCB);

        Vcb->TargetDeviceObject = TargetDeviceObject;
        Vcb->DiskGeometry = DiskGeometry;

        Status = Ext2LoadSuper(Vcb, FALSE, &Ext2Sb);

        if (!NT_SUCCESS(Status)) {
            Vcb = NULL;
            Status = STATUS_UNRECOGNIZED_VOLUME;
            __leave;
        }
        ASSERT (NULL != Ext2Sb);

        /* check Linux Ext2/Ext3 volume magic */
        if (Ext2Sb->s_magic == EXT2_SUPER_MAGIC) {
            DEBUG(DL_INF, ( "Volume of ext2 file system is found.\n"));
        } else  {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            Vcb = NULL;
            __leave;
        }

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
                if (OldVpb == Vpb) {continue;}

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
            Ext2InsertVcb(Vcb);
            Vcb = NULL; Vpb = NULL;
            ObDereferenceObject(TargetDeviceObject);

        } else {

            Vcb = NULL;
        }

    } __finally {

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
                    ExFreePoolWithTag(Ext2Sb, EXT2_SB_MAGIC);
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
    }
    
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

    __try {

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
        
    } __finally {

    }
  
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
    PIO_STACK_LOCATION      IoStackLocation;
    ULONG                   ChangeCount = 0;
    ULONG                   dwBytes;
    
    __try {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
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
            __leave;
        }

        if (!IsMounted(Vcb)) {
            Status = STATUS_WRONG_VOLUME;
            __leave;
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
            __leave;
        } else {
            Vcb->ChangeCount = ChangeCount;
        }
        
        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        Status = Ext2LoadSuper(Vcb, TRUE, &ext2_sb);

        if (!NT_SUCCESS(Status)) {
            __leave;
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

    } __finally {

        if (ext2_sb)
            ExFreePoolWithTag(ext2_sb, EXT2_SB_MAGIC);

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    }
    
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
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
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
            __leave;
        }

/*        
        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            DEBUG(DL_ERR, ( "Ext2Dismount: Volume is not locked.\n"));
            
            Status = STATUS_ACCESS_DENIED;
           
            __leave;
        }
*/
        Ext2FlushFiles(IrpContext, Vcb, FALSE);

        Ext2FlushVolume(IrpContext, Vcb, FALSE);

        Ext2PurgeVolume(Vcb, TRUE);

        ExReleaseResourceLite(&Vcb->MainResource);

        VcbResourceAcquired = FALSE;

        Ext2CheckDismount(IrpContext, Vcb, TRUE);

        DEBUG(DL_INF, ( "Ext2Dismount: Volume dismount pending.\n"));

        Status = STATUS_SUCCESS;

    } __finally {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }
        
        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext,  Status);
        }
    }
    
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
    BOOLEAN bDeleted = FALSE;
    ULONG   UnCleanCount = 0;

    NewVpb = ExAllocatePoolWithTag(NonPagedPool, VPB_SIZE, TAG_VPB);
    if (NewVpb == NULL) {
        DEBUG(DL_ERR, ( "Ex2CheckDismount: failed to allocate NewVpb.\n"));
        return FALSE;
    }
    INC_MEM_COUNT(PS_VPB, NewVpb, sizeof(VPB));
    memset(NewVpb, '_', VPB_SIZE);
    RtlZeroMemory(NewVpb, sizeof(VPB));

    ExAcquireResourceExclusiveLite(
        &Ext2Global->Resource, TRUE );

    ExAcquireResourceExclusiveLite(
        &Vcb->MainResource, TRUE );

    if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
        (IrpContext->RealDevice == Vcb->RealDevice)) {
        UnCleanCount = 3;
    } else {
        UnCleanCount = 2;
    }

    IoAcquireVpbSpinLock (&Irql);

    DEBUG(DL_DBG, ("Ext2CheckDismount: Vpb %p ioctl=%d Device %p\n",
                         Vpb, Vpb->ReferenceCount, Vpb->RealDevice));
    if (Vpb->ReferenceCount <= UnCleanCount) {

        ClearFlag(Vpb->Flags, VPB_MOUNTED);
        ClearFlag(Vpb->Flags, VPB_LOCKED);

        if ((Vcb->RealDevice != Vpb->RealDevice) &&
            (Vcb->RealDevice->Vpb == Vpb)) {
            SetFlag(Vcb->RealDevice->Flags, DO_DEVICE_INITIALIZING);
            SetFlag(Vpb->Flags, VPB_PERSISTENT );
        }

        Ext2RemoveVcb(Vcb);

        ClearFlag(Vpb->Flags, VPB_MOUNTED);
        SetLongFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);

        Vpb->DeviceObject = NULL;
        bDeleted = TRUE;

    } else if (bForce) {

        DEBUG(DL_DBG, ( "Ext2CheckDismount: NewVpb %p Realdevice = %p\n",
                                NewVpb, Vpb->RealDevice));

        NewVpb->Type = IO_TYPE_VPB;
        NewVpb->Size = sizeof( VPB );
        NewVpb->RealDevice = Vpb->RealDevice;
        NewVpb->RealDevice->Vpb = NewVpb;
        NewVpb->Flags = FlagOn(Vpb->Flags, VPB_REMOVE_PENDING);
        NewVpb = NULL;

        SetFlag(Vcb->Flags, VCB_NEW_VPB);
        ClearFlag(Vcb->Flags, VCB_MOUNTED);
    }

    IoReleaseVpbSpinLock(Irql);

    ExReleaseResourceLite(&Vcb->MainResource);
    ExReleaseResourceLite(&Ext2Global->Resource);

    if (bDeleted) {
        DEBUG(DL_DBG, ( "Ext2CheckDismount: Deleting vcb %p ...\n", Vcb));
        Ext2DestroyVcb(Vcb);
    }

    if (NewVpb != NULL) {
        DEBUG(DL_DBG, ( "Ext2CheckDismount: freeing Vpb %p\n", NewVpb));
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
    LIST_ENTRY      FcbList;
    PLIST_ENTRY     ListEntry;
    PFCB_LIST_ENTRY FcbListEntry;
   
    __try {

        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        
        if ( IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
             IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
            FlushBeforePurge = FALSE;
        }

        FcbListEntry= NULL;
        InitializeListHead(&FcbList);
        
        for (ListEntry = Vcb->FcbList.Flink;
             ListEntry != &Vcb->FcbList;
             ListEntry = ListEntry->Flink  ) {

            Fcb = CONTAINING_RECORD(ListEntry, EXT2_FCB, Next);

            DEBUG(DL_INF, ( "Ext2PurgeVolume: %wZ refercount=%xh\n", &Fcb->Mcb->FullName, Fcb->ReferenceCount));
            
            FcbListEntry = ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof(FCB_LIST_ENTRY),
                                EXT2_FLIST_MAGIC
                            );

            if (FcbListEntry) {
                FcbListEntry->Fcb = Fcb;
                Ext2ReferXcb(&Fcb->ReferenceCount);
                InsertTailList(&FcbList, &FcbListEntry->Next);
            } else {
                DEBUG(DL_ERR, ( "Ext2PurgeVolume: failed to allocate FcbListEntry ...\n"));
            }
        }
        
        while (!IsListEmpty(&FcbList)) {

            ListEntry = RemoveHeadList(&FcbList);
            
            FcbListEntry = CONTAINING_RECORD(ListEntry, FCB_LIST_ENTRY, Next);
            
            Fcb = FcbListEntry->Fcb;

            if (ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                TRUE )) {

                Ext2PurgeFile(Fcb, FlushBeforePurge);

                if (!Fcb->OpenHandleCount && Fcb->ReferenceCount == 1) {
                    Ext2FreeFcb(Fcb);
                } else {
                    ExReleaseResourceLite(&Fcb->MainResource);
                }
            }
           
            ExFreePoolWithTag(FcbListEntry, EXT2_FLIST_MAGIC);
        }

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

    } __finally {
        // Nothing
    }

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

    
    if( !IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) && FlushBeforePurge &&
        !IsFlagOn(Fcb->Vcb->Flags, VCB_WRITE_PROTECTED)) {

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

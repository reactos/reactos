/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             devctl.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   13 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 *                     Fixed some warnings under GCC
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

NTSTATUS
Ext2DeviceControlCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context);

VOID
Ext2DeviceControlNormalFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN BOOLEAN              CompleteRequest,
    IN PNTSTATUS            pStatus);

/* Also used by Ex2ProcessMountPoint() */
VOID
Ext2ProcessUserPropertyFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus);

VOID
Ex2ProcessUserPerfStatFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN BOOLEAN              GlobalDataResourceAcquired);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2DeviceControl)
#pragma alloc_text(PAGE, Ext2DeviceControlNormal)
#pragma alloc_text(PAGE, Ext2ProcessVolumeProperty)
#pragma alloc_text(PAGE, Ext2ProcessUserProperty)
#pragma alloc_text(PAGE, Ext2ProcessGlobalProperty)
#pragma alloc_text(PAGE, Ex2ProcessUserPerfStat)
#pragma alloc_text(PAGE, Ex2ProcessMountPoint)
#if EXT2_UNLOAD
#pragma alloc_text(PAGE, Ext2PrepareToUnload)
#endif
#endif


/* FUNCTIONS ***************************************************************/


NTSTATUS
Ext2DeviceControlCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}

_SEH_DEFINE_LOCALS(Ext2DeviceControlNormalFinal)
{
    IN PEXT2_IRP_CONTEXT    IrpContext;
    IN BOOLEAN              CompleteRequest;
    IN PNTSTATUS            pStatus;
};

_SEH_FINALLYFUNC(Ext2DeviceControlNormalFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2DeviceControlNormalFinal);
    Ext2DeviceControlNormalFinal(_SEH_VAR(IrpContext), _SEH_VAR(CompleteRequest),
                                 _SEH_VAR(pStatus));
}

VOID
Ext2DeviceControlNormalFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN BOOLEAN              CompleteRequest,
    IN PNTSTATUS            pStatus
    )
{
    if (!IrpContext->ExceptionInProgress) {
        if (IrpContext) {
            if (!CompleteRequest) {
                IrpContext->Irp = NULL;
            }

            Ext2CompleteIrpContext(IrpContext, *pStatus);
        }
    }
}

NTSTATUS
Ext2DeviceControlNormal (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB       Vcb;

    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PDEVICE_OBJECT  TargetDeviceObject;
    
    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2DeviceControlNormalFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(CompleteRequest) = TRUE;
        _SEH_VAR(pStatus) = &Status;

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        _SEH_VAR(CompleteRequest) = TRUE;

        DeviceObject = IrpContext->DeviceObject;
    
        if (IsExt2FsDevice(DeviceObject))  {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }
        
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        Vcb = (PEXT2_VCB) IrpSp->FileObject->FsContext;

        if (!((Vcb) && (Vcb->Identifier.Type == EXT2VCB) &&
              (Vcb->Identifier.Size == sizeof(EXT2_VCB)))) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }
        
        TargetDeviceObject = Vcb->TargetDeviceObject;
        
        //
        // Pass on the IOCTL to the driver below
        //
        
        _SEH_VAR(CompleteRequest) = FALSE;

        NextIrpSp = IoGetNextIrpStackLocation( Irp );
        *NextIrpSp = *IrpSp;

        IoSetCompletionRoutine(
            Irp,
            Ext2DeviceControlCompletion,
            NULL,
            FALSE,
            TRUE,
            TRUE );
        
        Status = IoCallDriver(TargetDeviceObject, Irp);

    }
    _SEH_FINALLY(Ext2DeviceControlNormalFinal_PSEH)
    _SEH_END;
    
    return Status;
}


#if EXT2_UNLOAD

NTSTATUS
Ext2PrepareToUnload (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    BOOLEAN         GlobalDataResourceAcquired = FALSE;
    
    __try {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }
        
        ExAcquireResourceExclusiveLite(
            &Ext2Global->Resource,
            TRUE );
        
        GlobalDataResourceAcquired = TRUE;
        
        if (FlagOn(Ext2Global->Flags, EXT2_UNLOAD_PENDING)) {
            DEBUG(DL_ERR, ( "Ext2PrepareUnload:  Already ready to unload.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        {
            PEXT2_VCB               Vcb;
            PLIST_ENTRY             ListEntry;

            ListEntry = Ext2Global->VcbList.Flink;

            while (ListEntry != &(Ext2Global->VcbList)) {

                Vcb = CONTAINING_RECORD(ListEntry, EXT2_VCB, Next);
                ListEntry = ListEntry->Flink;

                if (Vcb && (!Vcb->ReferenceCount) &&
                    IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                    Ext2RemoveVcb(Vcb);
                    Ext2ClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

                    Ext2DestroyVcb(Vcb);
                }
            }
        }

        if (!IsListEmpty(&(Ext2Global->VcbList))) {

            DEBUG(DL_ERR, ( "Ext2PrepareUnload:  Mounted volumes exists.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        IoUnregisterFileSystem(Ext2Global->DiskdevObject);
        IoUnregisterFileSystem(Ext2Global->CdromdevObject);
        Ext2Global->DriverObject->DriverUnload = DriverUnload;
        SetLongFlag(Ext2Global->Flags ,EXT2_UNLOAD_PENDING);
        Status = STATUS_SUCCESS;

        DEBUG(DL_INF, ( "Ext2PrepareToUnload: Driver is ready to unload.\n"));

    } __finally {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }
        
        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    }
    
    return Status;
}

#endif

extern CHAR gVersion[];
extern CHAR gTime[];
extern CHAR gDate[];

NTSTATUS
Ext2ProcessGlobalProperty(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PEXT2_VOLUME_PROPERTY2 Property
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    BOOLEAN         GlobalDataResourceAcquired = FALSE;
    struct nls_table * PageTable = NULL;

    if (!IsFlagOn(Property->Flags, EXT2_FLAG_VP_SET_GLOBAL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto errorout;
    }

    /* query Ext2Fsd's version and built date/time*/
    if (Property->Command == APP_CMD_QUERY_VERSION) {

        PEXT2_VOLUME_PROPERTY_VERSION PVPV =
            (PEXT2_VOLUME_PROPERTY_VERSION) Property;
        RtlZeroMemory(&PVPV->Date[0],   0x20);
        RtlZeroMemory(&PVPV->Time[0],   0x20);
        RtlZeroMemory(&PVPV->Version[0],0x1C);
        strncpy(&PVPV->Version[0], gVersion, 0x1B);
        strncpy(&PVPV->Date[0], gDate, 0x1F);
        strncpy(&PVPV->Time[0], gTime, 0x1F);
        goto errorout;
    }

    /* must be property query/set commands */
    if (Property->Command != APP_CMD_SET_PROPERTY &&
        Property->Command != APP_CMD_SET_PROPERTY2 ) {
        Status = STATUS_INVALID_PARAMETER;
        goto errorout;
    }

    ExAcquireResourceExclusiveLite(&Ext2Global->Resource, TRUE);
    GlobalDataResourceAcquired = TRUE;

    if (Property->bReadonly) {
        ClearLongFlag(Ext2Global->Flags, EXT2_SUPPORT_WRITING);
        ClearLongFlag(Ext2Global->Flags, EXT3_FORCE_WRITING);
    } else {
        SetLongFlag(Ext2Global->Flags, EXT2_SUPPORT_WRITING);
        if (Property->bExt3Writable) {
            SetLongFlag(Ext2Global->Flags, EXT3_FORCE_WRITING);
        } else {
            ClearLongFlag(Ext2Global->Flags, EXT3_FORCE_WRITING);
        }
    }

    PageTable = load_nls(Property->Codepage);
    if (PageTable) {
        memcpy(Ext2Global->Codepage.AnsiName, Property->Codepage, CODEPAGE_MAXLEN);
        Ext2Global->Codepage.PageTable = PageTable;
    }

    if (Property->Command == APP_CMD_SET_PROPERTY2) {
        RtlZeroMemory(Ext2Global->sHidingPrefix, HIDINGPAT_LEN);
        if ((Ext2Global->bHidingPrefix = Property->bHidingPrefix)) {
            RtlCopyMemory( Ext2Global->sHidingPrefix,
                           Property->sHidingPrefix,
                           HIDINGPAT_LEN - 1);
        }
        RtlZeroMemory(Ext2Global->sHidingSuffix, HIDINGPAT_LEN);
        if ((Ext2Global->bHidingSuffix = Property->bHidingSuffix)) {
            RtlCopyMemory( Ext2Global->sHidingSuffix,
                           Property->sHidingSuffix,
                           HIDINGPAT_LEN - 1);
        }
    }

errorout:

    if (GlobalDataResourceAcquired) {
        ExReleaseResourceLite(&Ext2Global->Resource);
    }

    return Status;
}


NTSTATUS
Ext2ProcessVolumeProperty(
    IN  PEXT2_VCB              Vcb,
    IN  PEXT2_VOLUME_PROPERTY2 Property
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    BOOLEAN         VcbResourceAcquired = FALSE;
    struct nls_table * PageTable = NULL;

    ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
    VcbResourceAcquired = TRUE;

    if (Property->Command == APP_CMD_SET_PROPERTY ||
        Property->Command == APP_CMD_SET_PROPERTY2 ) {

        if (Property->bReadonly) {

            Ext2FlushFiles(NULL, Vcb, FALSE);
            Ext2FlushVolume(NULL, Vcb, FALSE);
            SetLongFlag(Vcb->Flags, VCB_READ_ONLY);

        } else {

            if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
            } else if (!Vcb->IsExt3fs) {
                ClearLongFlag(Vcb->Flags, VCB_READ_ONLY);
            } else if (!Property->bExt3Writable) {
                SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
            } else if (IsFlagOn(Vcb->Flags, VCB_JOURNAL_RECOVER)) {
                ClearLongFlag(Vcb->Flags, VCB_READ_ONLY);
                Ext2RecoverJournal(NULL, Vcb);
                if (IsFlagOn(Vcb->Flags, VCB_JOURNAL_RECOVER)) {
                    SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
                } else {
                    ClearLongFlag(Vcb->Flags, VCB_READ_ONLY);
                }
            } else {
                ClearLongFlag(Vcb->Flags, VCB_READ_ONLY);
            }
        }

        PageTable = load_nls(Property->Codepage);
        memcpy(Vcb->Codepage.AnsiName, Property->Codepage, CODEPAGE_MAXLEN);
        Vcb->Codepage.PageTable = PageTable;
        if (Vcb->Codepage.PageTable) {
            Ext2InitializeLabel(Vcb, Vcb->SuperBlock);
        }

        if (Property->Command == APP_CMD_SET_PROPERTY2) {

            RtlZeroMemory(Vcb->sHidingPrefix, HIDINGPAT_LEN);
            if (Vcb->bHidingPrefix = Property->bHidingPrefix) {
                RtlCopyMemory( Vcb->sHidingPrefix,
                               Property->sHidingPrefix,
                               HIDINGPAT_LEN - 1);
            }

            RtlZeroMemory(Vcb->sHidingSuffix, HIDINGPAT_LEN);
            if (Vcb->bHidingSuffix = Property->bHidingSuffix) {
                RtlCopyMemory( Vcb->sHidingSuffix,
                               Property->sHidingSuffix,
                               HIDINGPAT_LEN - 1);
            }

            Vcb->DrvLetter = Property->DrvLetter;
        }

    } else if (Property->Command == APP_CMD_QUERY_PROPERTY ||
               Property->Command == APP_CMD_QUERY_PROPERTY2 ) {

        Property->bExt2 = TRUE;
        Property->bExt3 = Vcb->IsExt3fs;
        Property->bReadonly = IsFlagOn(Vcb->Flags, VCB_READ_ONLY);
        if (!Property->bReadonly && Vcb->IsExt3fs) {
            Property->bExt3Writable = TRUE;
        } else {
            Property->bExt3Writable = FALSE;
        }

        RtlZeroMemory(Property->Codepage, CODEPAGE_MAXLEN);
        if (Vcb->Codepage.PageTable) {
            strcpy(Property->Codepage, Vcb->Codepage.PageTable->charset);
        } else {
            strcpy(Property->Codepage, "default");
        }

        if (Property->Command == APP_CMD_QUERY_PROPERTY2) {

            RtlCopyMemory(Property->UUID, Vcb->SuperBlock->s_uuid, 16);

            Property->DrvLetter = Vcb->DrvLetter;

            if (Property->bHidingPrefix = Vcb->bHidingPrefix) {
                RtlCopyMemory( Property->sHidingPrefix,
                               Vcb->sHidingPrefix,
                               HIDINGPAT_LEN);
            } else {
                RtlZeroMemory( Property->sHidingPrefix,
                               HIDINGPAT_LEN);
            }

            if (Property->bHidingSuffix = Vcb->bHidingSuffix) {
                RtlCopyMemory( Property->sHidingSuffix,
                               Vcb->sHidingSuffix,
                               HIDINGPAT_LEN);
            } else {
                RtlZeroMemory( Property->sHidingSuffix,
                               HIDINGPAT_LEN);
            }
        }

    } else {

        Status = STATUS_INVALID_PARAMETER;
        goto errorout;
    }

errorout:

    if (VcbResourceAcquired) {
        ExReleaseResourceLite(&Vcb->MainResource);
    }
       
    return Status;
}

_SEH_DEFINE_LOCALS(Ext2ProcessUserPropertyFinal)
{
    IN PEXT2_IRP_CONTEXT    IrpContext;
    IN PNTSTATUS            pStatus;
};

_SEH_FINALLYFUNC(Ext2ProcessUserPropertyFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2DeviceControlNormalFinal);
    Ext2ProcessUserPropertyFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus));
}

VOID
Ext2ProcessUserPropertyFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus
    )
{
    if (!IrpContext->ExceptionInProgress) {
        Ext2CompleteIrpContext(IrpContext, *pStatus);
    }
}

NTSTATUS
Ext2ProcessUserProperty(
    IN PEXT2_IRP_CONTEXT        IrpContext,
    IN PEXT2_VOLUME_PROPERTY2   Property,
    IN ULONG                    Length
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PEXT2_VCB   Vcb = NULL;
    PDEVICE_OBJECT  DeviceObject = NULL;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2ProcessUserPropertyFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        if (Property->Magic != EXT2_VOLUME_PROPERTY_MAGIC) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {
            Status = Ext2ProcessGlobalProperty(DeviceObject, Property);
        } else {
            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
            if (!((Vcb) && (Vcb->Identifier.Type == EXT2VCB) &&
                           (Vcb->Identifier.Size == sizeof(EXT2_VCB)))) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH_LEAVE;
            }
            Status = Ext2ProcessVolumeProperty(Vcb, Property);
        }

        if (NT_SUCCESS(Status)) {
            IrpContext->Irp->IoStatus.Information = Length;
        }

    }
    _SEH_FINALLY(Ext2ProcessUserPropertyFinal_PSEH)
    _SEH_END;
    
    return Status;
}

_SEH_DEFINE_LOCALS(Ex2ProcessUserPerfStatFinal)
{
    IN PEXT2_IRP_CONTEXT    IrpContext;
    IN PNTSTATUS            pStatus;
    IN BOOLEAN              GlobalDataResourceAcquired;
};

_SEH_FINALLYFUNC(Ex2ProcessUserPerfStatFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ex2ProcessUserPerfStatFinal);
    Ex2ProcessUserPerfStatFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus),
                                _SEH_VAR(GlobalDataResourceAcquired));
}

VOID
Ex2ProcessUserPerfStatFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN BOOLEAN              GlobalDataResourceAcquired
    )
{
    if (GlobalDataResourceAcquired) {
        ExReleaseResourceLite(&Ext2Global->Resource);
    }

    if (!IrpContext->ExceptionInProgress) {
        Ext2CompleteIrpContext(IrpContext, *pStatus);
    }
}

NTSTATUS
Ex2ProcessUserPerfStat(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_QUERY_PERFSTAT QueryPerf,
    IN ULONG                Length
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    PDEVICE_OBJECT  DeviceObject = NULL;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ex2ProcessUserPerfStatFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(GlobalDataResourceAcquired) = FALSE;

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {

            if (QueryPerf->Magic != EXT2_QUERY_PERFSTAT_MAGIC) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH_LEAVE;
            }

            if (QueryPerf->Command != IOCTL_APP_QUERY_PERFSTAT) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH_LEAVE;
            }

            if (Length < sizeof(EXT2_QUERY_PERFSTAT)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH_LEAVE;
            }

            ExAcquireResourceSharedLite(&Ext2Global->Resource, TRUE);
            _SEH_VAR(GlobalDataResourceAcquired) = TRUE;

            QueryPerf->PerfStat = Ext2Global->PerfStat;

        } else {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        if (NT_SUCCESS(Status)) {
            IrpContext->Irp->IoStatus.Information = Length;
        }

    }
    _SEH_FINALLY(Ex2ProcessUserPerfStatFinal_PSEH)
    _SEH_END;
    
    return Status;
}

_SEH_DEFINE_LOCALS(Ex2ProcessMountPointFinal)
{
    IN PEXT2_IRP_CONTEXT    IrpContext;
    IN PNTSTATUS            pStatus;
};

/* Use Ext2ProcessUserProperty() PSEH final function */
_SEH_FINALLYFUNC(Ex2ProcessMountPointFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ex2ProcessMountPointFinal);
    Ext2ProcessUserPropertyFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus));
}

NTSTATUS
Ex2ProcessMountPoint(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_MOUNT_POINT    MountPoint,
    IN ULONG                Length
    )
{
    UNICODE_STRING  Link;
    UNICODE_STRING  Target;
    WCHAR           Buffer[] = L"\\DosDevices\\Z:";
    NTSTATUS        status = STATUS_SUCCESS;

    PDEVICE_OBJECT  DeviceObject = NULL;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ex2ProcessMountPointFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &status;

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (!IsExt2FsDevice(DeviceObject)) {
            status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        if (Length != sizeof(EXT2_MOUNT_POINT) ||
            MountPoint->Magic != EXT2_APP_MOUNTPOINT_MAGIC) {
            status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        RtlInitUnicodeString(&Link, Buffer);
        Buffer[12] = MountPoint->Link[0];

        switch (MountPoint->Command) {

            case APP_CMD_ADD_DOS_SYMLINK:
                RtlInitUnicodeString(&Target, &MountPoint->Name[0]);
                status = IoCreateSymbolicLink(&Link, &Target);
                break;

            case APP_CMD_DEL_DOS_SYMLINK:
                status = IoDeleteSymbolicLink(&Link);
                break;

            default:
                status = STATUS_INVALID_PARAMETER;
        }

    }
    _SEH_FINALLY(Ex2ProcessMountPointFinal_PSEH)
    _SEH_END;

    return status;
}

NTSTATUS
Ext2DeviceControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  irpSp;
    ULONG               code;
    ULONG               length;
    NTSTATUS            Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    Irp = IrpContext->Irp;
    
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    
    code = irpSp->Parameters.DeviceIoControl.IoControlCode;
    length = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
    switch (code) {

    case IOCTL_APP_VOLUME_PROPERTY:
        Status = Ext2ProcessUserProperty(
                        IrpContext,
                        Irp->AssociatedIrp.SystemBuffer,
                        length
                        );
        break;

    case IOCTL_APP_QUERY_PERFSTAT:
        Status = Ex2ProcessUserPerfStat(
                        IrpContext,
                        Irp->AssociatedIrp.SystemBuffer,
                        length
                        );
         break;

    case IOCTL_APP_MOUNT_POINT:
        Status = Ex2ProcessMountPoint(
                        IrpContext,
                        Irp->AssociatedIrp.SystemBuffer,
                        length
                        );
         break;

#if EXT2_UNLOAD
    case IOCTL_PREPARE_TO_UNLOAD:
        Status = Ext2PrepareToUnload(IrpContext);
        break;
#endif
    default:
        Status = Ext2DeviceControlNormal(IrpContext);
    }
    
    return Status;
}

/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             devctl.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

NTSTATUS NTAPI
Ext2DeviceControlCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context);


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


NTSTATUS NTAPI
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


NTSTATUS
Ext2DeviceControlNormal (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteRequest = TRUE;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB       Vcb;

    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PDEVICE_OBJECT  TargetDeviceObject;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        CompleteRequest = TRUE;

        DeviceObject = IrpContext->DeviceObject;

        if (IsExt2FsDevice(DeviceObject))  {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        Vcb = (PEXT2_VCB) IrpSp->FileObject->FsContext;

        if (!((Vcb) && (Vcb->Identifier.Type == EXT2VCB) &&
                (Vcb->Identifier.Size == sizeof(EXT2_VCB)))) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        TargetDeviceObject = Vcb->TargetDeviceObject;

        //
        // Pass on the IOCTL to the driver below
        //

        CompleteRequest = FALSE;

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

    } _SEH2_FINALLY  {

        if (!IrpContext->ExceptionInProgress) {
            if (IrpContext) {
                if (!CompleteRequest) {
                    IrpContext->Irp = NULL;
                }

                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}


#if EXT2_UNLOAD

NTSTATUS
Ext2PrepareToUnload (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    BOOLEAN         GlobalDataResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        ExAcquireResourceExclusiveLite(
            &Ext2Global->Resource,
            TRUE );

        GlobalDataResourceAcquired = TRUE;

        if (FlagOn(Ext2Global->Flags, EXT2_UNLOAD_PENDING)) {
            DEBUG(DL_ERR, ( "Ext2PrepareUnload:  Already ready to unload.\n"));

            Status = STATUS_ACCESS_DENIED;

            _SEH2_LEAVE;
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

            _SEH2_LEAVE;
        }

        IoUnregisterFileSystem(Ext2Global->DiskdevObject);
        IoUnregisterFileSystem(Ext2Global->CdromdevObject);
        Ext2Global->DriverObject->DriverUnload = DriverUnload;
        SetLongFlag(Ext2Global->Flags ,EXT2_UNLOAD_PENDING);
        Status = STATUS_SUCCESS;

        DEBUG(DL_INF, ( "Ext2PrepareToUnload: Driver is ready to unload.\n"));

    } _SEH2_FINALLY {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}

#endif

extern CHAR gVersion[];
extern CHAR gTime[];
extern CHAR gDate[];

NTSTATUS
Ext2ProcessGlobalProperty(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PEXT2_VOLUME_PROPERTY3 Property3,
    IN  ULONG Length
)
{
    PEXT2_VOLUME_PROPERTY3 Property2 = (PVOID)Property3;
    PEXT2_VOLUME_PROPERTY  Property = (PVOID)Property3;
    struct nls_table * PageTable = NULL;

    NTSTATUS        Status = STATUS_SUCCESS;
    BOOLEAN         GlobalDataResourceAcquired = FALSE;

    _SEH2_TRY {

        if (Length < 8 || !IsFlagOn(Property->Flags, EXT2_FLAG_VP_SET_GLOBAL)) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        /* query Ext2Fsd's version and built date/time*/
        if (Property->Command == APP_CMD_QUERY_VERSION) {
            PEXT2_VOLUME_PROPERTY_VERSION PVPV =
                (PEXT2_VOLUME_PROPERTY_VERSION) Property;

            if (Length < sizeof(EXT2_VOLUME_PROPERTY_VERSION)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            RtlZeroMemory(&PVPV->Date[0],   0x20);
            RtlZeroMemory(&PVPV->Time[0],   0x20);
            RtlZeroMemory(&PVPV->Version[0],0x1C);
            strncpy(&PVPV->Version[0], gVersion, 0x1B);
            strncpy(&PVPV->Date[0], gDate, 0x1F);
            strncpy(&PVPV->Time[0], gTime, 0x1F);
            _SEH2_LEAVE;
        }

        /* must be property query/set commands */
        if (Property->Command == APP_CMD_SET_PROPERTY) {
            if (Length < sizeof(EXT2_VOLUME_PROPERTY)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        } else if (Property->Command == APP_CMD_SET_PROPERTY2) {
            if (Length < sizeof(EXT2_VOLUME_PROPERTY2)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        } else if (Property->Command == APP_CMD_SET_PROPERTY3) {
            if (Length < sizeof(EXT2_VOLUME_PROPERTY3)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        } else {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ExAcquireResourceExclusiveLite(&Ext2Global->Resource, TRUE);
        GlobalDataResourceAcquired = TRUE;


        switch (Property->Command) {

            case APP_CMD_SET_PROPERTY3:

            if (Property3->Flags2 & EXT2_VPROP3_AUTOMOUNT) {
                if (Property3->AutoMount)
                    SetLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);
                else
                    ClearLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);
            }

            case APP_CMD_SET_PROPERTY2:

            RtlZeroMemory(Ext2Global->sHidingPrefix, HIDINGPAT_LEN);
            if ((Ext2Global->bHidingPrefix = Property2->bHidingPrefix)) {
                RtlCopyMemory( Ext2Global->sHidingPrefix,
                               Property2->sHidingPrefix,
                               HIDINGPAT_LEN - 1);
            }
            RtlZeroMemory(Ext2Global->sHidingSuffix, HIDINGPAT_LEN);
            if ((Ext2Global->bHidingSuffix = Property2->bHidingSuffix)) {
                RtlCopyMemory( Ext2Global->sHidingSuffix,
                               Property2->sHidingSuffix,
                               HIDINGPAT_LEN - 1);
            }

            case APP_CMD_SET_PROPERTY:

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

            break;

            default:
            break;
        }

    } _SEH2_FINALLY {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2ProcessVolumeProperty(
    IN  PEXT2_VCB              Vcb,
    IN  PEXT2_VOLUME_PROPERTY3 Property3,
    IN  ULONG Length
)
{
    struct nls_table * PageTable = NULL;
    PEXT2_VOLUME_PROPERTY2 Property2 = (PVOID)Property3;
    PEXT2_VOLUME_PROPERTY  Property = (PVOID)Property3;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN VcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
        VcbResourceAcquired = TRUE;

        if (Property->Command == APP_CMD_SET_PROPERTY ||
            Property->Command == APP_CMD_QUERY_PROPERTY) {
            if (Length < sizeof(EXT2_VOLUME_PROPERTY)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        } else if (Property->Command == APP_CMD_SET_PROPERTY2 ||
                   Property->Command == APP_CMD_QUERY_PROPERTY2) {
            if (Length < sizeof(EXT2_VOLUME_PROPERTY2)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        } else if (Property->Command == APP_CMD_SET_PROPERTY3 ||
                   Property->Command == APP_CMD_QUERY_PROPERTY3) {
            if (Length < sizeof(EXT2_VOLUME_PROPERTY3)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        }

        switch (Property->Command) {

        case APP_CMD_SET_PROPERTY3:

            if (Property3->Flags2 & EXT2_VPROP3_AUTOMOUNT) {
                if (Property3->AutoMount)
                    SetLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);
                else
                    ClearLongFlag(Ext2Global->Flags, EXT2_AUTO_MOUNT);
            }
            if (Property3->Flags2 & EXT2_VPROP3_USERIDS) {
                SetFlag(Vcb->Flags, VCB_USER_IDS);
                Vcb->uid = Property3->uid;
                Vcb->gid = Property3->gid;
                if (Property3->EIDS) {
                    Vcb->euid = Property3->euid;
                    Vcb->egid = Property3->egid;
                    SetFlag(Vcb->Flags, VCB_USER_EIDS);
                } else {
                    Vcb->euid = Vcb->egid = 0;
                    ClearFlag(Vcb->Flags, VCB_USER_EIDS);
                }
            } else {
                ClearFlag(Vcb->Flags, VCB_USER_IDS);
                ClearFlag(Vcb->Flags, VCB_USER_EIDS);
                Vcb->uid = Vcb->gid = 0;
                Vcb->euid = Vcb->egid = 0;
            }

        case APP_CMD_SET_PROPERTY2:

            RtlZeroMemory(Vcb->sHidingPrefix, HIDINGPAT_LEN);
            if (Vcb->bHidingPrefix == Property2->bHidingPrefix) {
                RtlCopyMemory( Vcb->sHidingPrefix,
                               Property2->sHidingPrefix,
                               HIDINGPAT_LEN - 1);
            }

            RtlZeroMemory(Vcb->sHidingSuffix, HIDINGPAT_LEN);
            if (Vcb->bHidingSuffix == Property2->bHidingSuffix) {
                RtlCopyMemory( Vcb->sHidingSuffix,
                               Property2->sHidingSuffix,
                               HIDINGPAT_LEN - 1);
            }
            Vcb->DrvLetter = Property2->DrvLetter;

        case APP_CMD_SET_PROPERTY:

            if (Property->bReadonly) {
                if (IsFlagOn(Vcb->Flags, VCB_INITIALIZED)) {
                    Ext2FlushFiles(NULL, Vcb, FALSE);
                    Ext2FlushVolume(NULL, Vcb, FALSE);
                }
                SetLongFlag(Vcb->Flags, VCB_READ_ONLY);

            } else {

                if (Property->bExt3Writable) {
                    SetLongFlag(Vcb->Flags, VCB_FORCE_WRITING);
                }

                if (!Vcb->IsExt3fs) {
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

            break;

        case APP_CMD_QUERY_PROPERTY3:

            if (IsFlagOn(Ext2Global->Flags, EXT2_AUTO_MOUNT)) {
                SetFlag(Property3->Flags2, EXT2_VPROP3_AUTOMOUNT);
                Property3->AutoMount = TRUE;
            } else {
                ClearFlag(Property3->Flags2, EXT2_VPROP3_AUTOMOUNT);
                Property3->AutoMount = FALSE;
            }

            if (IsFlagOn(Vcb->Flags, VCB_USER_IDS)) {
                SetFlag(Property3->Flags2, EXT2_VPROP3_USERIDS);
                Property3->uid = Vcb->uid;
                Property3->gid = Vcb->gid;
                if (IsFlagOn(Vcb->Flags, VCB_USER_EIDS)) {
                    Property3->EIDS = TRUE;
                    Property3->euid = Vcb->euid;
                    Property3->egid = Vcb->egid;
                } else {
                    Property3->EIDS = FALSE;
                }
            } else {
                ClearFlag(Property3->Flags2, EXT2_VPROP3_USERIDS);
            }

        case APP_CMD_QUERY_PROPERTY2:

            RtlCopyMemory(Property2->UUID, Vcb->SuperBlock->s_uuid, 16);
            Property2->DrvLetter = Vcb->DrvLetter;

            if (Property2->bHidingPrefix == Vcb->bHidingPrefix) {
                RtlCopyMemory( Property2->sHidingPrefix,
                               Vcb->sHidingPrefix,
                               HIDINGPAT_LEN);
            } else {
                RtlZeroMemory( Property2->sHidingPrefix,
                               HIDINGPAT_LEN);
            }

            if (Property2->bHidingSuffix == Vcb->bHidingSuffix) {
                RtlCopyMemory( Property2->sHidingSuffix,
                               Vcb->sHidingSuffix,
                               HIDINGPAT_LEN);
            } else {
                RtlZeroMemory( Property2->sHidingSuffix,
                               HIDINGPAT_LEN);
            }

        case APP_CMD_QUERY_PROPERTY:

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
                strncpy(Property->Codepage, Vcb->Codepage.PageTable->charset, CODEPAGE_MAXLEN);
            } else {
                strncpy(Property->Codepage, "default", CODEPAGE_MAXLEN);
            }
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2ProcessUserProperty(
    IN PEXT2_IRP_CONTEXT        IrpContext,
    IN PEXT2_VOLUME_PROPERTY3   Property,
    IN ULONG                    Length
)
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PEXT2_VCB   Vcb = NULL;
    PDEVICE_OBJECT  DeviceObject = NULL;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        if (Property->Magic != EXT2_VOLUME_PROPERTY_MAGIC) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {
            Status = Ext2ProcessGlobalProperty(DeviceObject, Property, Length);
        } else {
            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
            if (!((Vcb) && (Vcb->Identifier.Type == EXT2VCB) &&
                    (Vcb->Identifier.Size == sizeof(EXT2_VCB)))) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
            Status = Ext2ProcessVolumeProperty(Vcb, Property, Length);
        }

        if (NT_SUCCESS(Status)) {
            IrpContext->Irp->IoStatus.Information = Length;
        }

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ex2ProcessUserPerfStat(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_QUERY_PERFSTAT QueryPerf,
    IN ULONG                Length
)
{

#ifndef __REACTOS__
    PEXT2_VCB   Vcb = NULL;
#endif
    PDEVICE_OBJECT  DeviceObject = NULL;

    BOOLEAN     GlobalDataResourceAcquired = FALSE;
    NTSTATUS    Status = STATUS_SUCCESS;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {

            if (QueryPerf->Magic != EXT2_QUERY_PERFSTAT_MAGIC) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            if (QueryPerf->Command != IOCTL_APP_QUERY_PERFSTAT) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            if (Length != EXT2_QUERY_PERFSTAT_SZV1 &&
                Length != EXT2_QUERY_PERFSTAT_SZV2) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ExAcquireResourceSharedLite(&Ext2Global->Resource, TRUE);
            GlobalDataResourceAcquired = TRUE;

            if (Length == EXT2_QUERY_PERFSTAT_SZV2) {
                QueryPerf->Flags = EXT2_QUERY_PERFSTAT_VER2;
                QueryPerf->PerfStatV2 = Ext2Global->PerfStat;
            } else {
                memcpy(&QueryPerf->PerfStatV1.Irps[0], &Ext2Global->PerfStat.Irps[0],
                       FIELD_OFFSET(EXT2_PERF_STATISTICS_V1, Unit));
                memcpy(&QueryPerf->PerfStatV1.Unit, &Ext2Global->PerfStat.Unit,
                       sizeof(EXT2_STAT_ARRAY_V1));
                memcpy(&QueryPerf->PerfStatV1.Current, &Ext2Global->PerfStat.Current,
                       sizeof(EXT2_STAT_ARRAY_V1));
                memcpy(&QueryPerf->PerfStatV1.Size, &Ext2Global->PerfStat.Size,
                       sizeof(EXT2_STAT_ARRAY_V1));
                memcpy(&QueryPerf->PerfStatV1.Total, &Ext2Global->PerfStat.Total,
                       sizeof(EXT2_STAT_ARRAY_V1));
            }

        } else {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (NT_SUCCESS(Status)) {
            IrpContext->Irp->IoStatus.Information = Length;
        }

    } _SEH2_FINALLY {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END

    return Status;
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
    WCHAR           Buffer[] = L"\\DosDevices\\Global\\Z:";
    NTSTATUS        status = STATUS_SUCCESS;

    PDEVICE_OBJECT  DeviceObject = NULL;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (!IsExt2FsDevice(DeviceObject)) {
            status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (Length != sizeof(EXT2_MOUNT_POINT) ||
            MountPoint->Magic != EXT2_APP_MOUNTPOINT_MAGIC) {
            status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
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

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, status);
        }
    } _SEH2_END;

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

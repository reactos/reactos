/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/create.c
 * PURPOSE:         Create routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
FatiCreate(IN PFAT_IRP_CONTEXT IrpContext,
           IN PIRP Irp)
{
    /* Boolean options */
    BOOLEAN CreateDirectory;
    BOOLEAN SequentialOnly;
    BOOLEAN NoIntermediateBuffering;
    BOOLEAN OpenDirectory;
    BOOLEAN IsPagingFile;
    BOOLEAN OpenTargetDirectory;
    BOOLEAN DirectoryFile;
    BOOLEAN NonDirectoryFile;
    BOOLEAN NoEaKnowledge;
    BOOLEAN DeleteOnClose;
    BOOLEAN TemporaryFile;
    ULONG CreateDisposition;

    /* Control blocks */
    PVCB Vcb, DecodedVcb;
    PFCB Fcb;
    PCCB Ccb;

    /* IRP data */
    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFO;
    UNICODE_STRING FileName;
    ULONG AllocationSize;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    PACCESS_MASK DesiredAccess;
    ULONG Options;
    UCHAR FileAttributes;
    USHORT ShareAccess;
    ULONG EaLength;

    /* Misc */
    NTSTATUS Status;
    //IO_STATUS_BLOCK Iosb;
    PIO_STACK_LOCATION IrpSp;

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("FatCommonCreate\n", 0 );
    DPRINT1("Irp                       = %08lx\n",   Irp );
    DPRINT1("\t->Flags                   = %08lx\n", Irp->Flags );
    DPRINT1("\t->FileObject              = %08lx\n", IrpSp->FileObject );
    DPRINT1("\t->RelatedFileObject       = %08lx\n", IrpSp->FileObject->RelatedFileObject );
    DPRINT1("\t->FileName                = %wZ\n",   &IrpSp->FileObject->FileName );
    DPRINT1("\t->AllocationSize.LowPart  = %08lx\n", Irp->Overlay.AllocationSize.LowPart );
    DPRINT1("\t->AllocationSize.HighPart = %08lx\n", Irp->Overlay.AllocationSize.HighPart );
    DPRINT1("\t->SystemBuffer            = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DPRINT1("\t->DesiredAccess           = %08lx\n", IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
    DPRINT1("\t->Options                 = %08lx\n", IrpSp->Parameters.Create.Options );
    DPRINT1("\t->FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DPRINT1("\t->ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DPRINT1("\t->EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength );

    /* Apply a special hack for Win32, idea taken from FASTFAT reference driver from WDK */
    if ((IrpSp->FileObject->FileName.Length > sizeof(WCHAR)) &&
        (IrpSp->FileObject->FileName.Buffer[1] == L'\\') &&
        (IrpSp->FileObject->FileName.Buffer[0] == L'\\'))
    {
        /* Remove a leading slash */
        IrpSp->FileObject->FileName.Length -= sizeof(WCHAR);
        RtlMoveMemory(&IrpSp->FileObject->FileName.Buffer[0],
                      &IrpSp->FileObject->FileName.Buffer[1],
                      IrpSp->FileObject->FileName.Length );

        /* Check again: if there are still two leading slashes,
           exit with an error */
        if ((IrpSp->FileObject->FileName.Length > sizeof(WCHAR)) &&
            (IrpSp->FileObject->FileName.Buffer[1] == L'\\') &&
            (IrpSp->FileObject->FileName.Buffer[0] == L'\\'))
        {
            FatCompleteRequest( IrpContext, Irp, STATUS_OBJECT_NAME_INVALID );

            DPRINT1("FatiCreate: STATUS_OBJECT_NAME_INVALID\n");
            return STATUS_OBJECT_NAME_INVALID;
        }
    }

    /* Make sure we have SecurityContext */
    ASSERT(IrpSp->Parameters.Create.SecurityContext != NULL);

    /* Get necessary data out of IRP */
    FileObject     = IrpSp->FileObject;
    FileName       = FileObject->FileName;
    RelatedFO      = FileObject->RelatedFileObject;
    AllocationSize = Irp->Overlay.AllocationSize.LowPart;
    EaBuffer       = Irp->AssociatedIrp.SystemBuffer;
    DesiredAccess  = &IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    Options        = IrpSp->Parameters.Create.Options;
    FileAttributes = (UCHAR)(IrpSp->Parameters.Create.FileAttributes & ~FILE_ATTRIBUTE_NORMAL);
    ShareAccess    = IrpSp->Parameters.Create.ShareAccess;
    EaLength       = IrpSp->Parameters.Create.EaLength;

    /* Set VPB to related object's VPB if it exists */
    if (RelatedFO)
        FileObject->Vpb = RelatedFO->Vpb;

    /* Prepare file attributes mask */
    FileAttributes &= (FILE_ATTRIBUTE_READONLY |
                       FILE_ATTRIBUTE_HIDDEN   |
                       FILE_ATTRIBUTE_SYSTEM   |
                       FILE_ATTRIBUTE_ARCHIVE);

    /* Get the volume control object */
    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    /* Get options */
    DirectoryFile           = BooleanFlagOn(Options, FILE_DIRECTORY_FILE);
    NonDirectoryFile        = BooleanFlagOn(Options, FILE_NON_DIRECTORY_FILE);
    SequentialOnly          = BooleanFlagOn(Options, FILE_SEQUENTIAL_ONLY);
    NoIntermediateBuffering = BooleanFlagOn(Options, FILE_NO_INTERMEDIATE_BUFFERING);
    NoEaKnowledge           = BooleanFlagOn(Options, FILE_NO_EA_KNOWLEDGE);
    DeleteOnClose           = BooleanFlagOn(Options, FILE_DELETE_ON_CLOSE);
    TemporaryFile           = BooleanFlagOn(IrpSp->Parameters.Create.FileAttributes,
                                            FILE_ATTRIBUTE_TEMPORARY );
    IsPagingFile            = BooleanFlagOn(IrpSp->Flags, SL_OPEN_PAGING_FILE);
    OpenTargetDirectory     = BooleanFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

    /* Calculate create disposition */
    CreateDisposition = (Options >> 24) & 0x000000ff;

    /* Get Create/Open directory flags based on it */
    CreateDirectory = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_CREATE) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    OpenDirectory   = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_OPEN) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    /* Validate parameters: directory/nondirectory mismatch and
       AllocationSize being more than 4GB */
    if ((DirectoryFile && NonDirectoryFile) ||
        Irp->Overlay.AllocationSize.HighPart != 0)
    {
        FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);

        DPRINT1("FatiCreate: STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    /* Acquire the VCB lock exclusively */
    if (!FatAcquireExclusiveVcb(IrpContext, Vcb))
    {
        // TODO: Postpone the IRP for later processing
        ASSERT(FALSE);
        return STATUS_NOT_IMPLEMENTED;
    }

    // TODO: Verify the VCB

    /* If VCB is locked, then no file openings are possible */
    if (Vcb->State & VCB_STATE_FLAG_LOCKED)
    {
        DPRINT1("This volume is locked\n");
        Status = STATUS_ACCESS_DENIED;

        /* Cleanup and return */
        FatReleaseVcb(IrpContext, Vcb);
        return Status;
    }

    // TODO: Check if the volume is write protected and disallow DELETE_ON_CLOSE

    // TODO: Make sure EAs aren't supported on FAT32

    if (FileName.Length == 0)
    {
        /* It is a volume open request, check related FO to be sure */

        if (!RelatedFO ||
            FatDecodeFileObject(RelatedFO, &DecodedVcb, &Fcb, &Ccb) == UserVolumeOpen)
        {
            /* It is indeed a volume open request */
            DPRINT1("Volume open request, not implemented now!\n");
            UNIMPLEMENTED;
        }
    }

    //return Iosb.Status;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFAT_IRP_CONTEXT IrpContext;
    NTSTATUS Status;
    //PVOLUME_DEVICE_OBJECT VolumeDO = (PVOLUME_DEVICE_OBJECT)DeviceObject;

    DPRINT1("FatCreate()\n");

    /* If it's called with our Disk FS device object - it's always open */
    // TODO: Add check for CDROM FS device object
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
    Status = FatiCreate(IrpContext, Irp);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

/* EOF */

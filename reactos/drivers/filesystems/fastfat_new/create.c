/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/create.c
 * PURPOSE:         Create routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

IO_STATUS_BLOCK
NTAPI
FatiOpenRootDcb(IN PFAT_IRP_CONTEXT IrpContext,
                IN PFILE_OBJECT FileObject,
                IN PVCB Vcb,
                IN PACCESS_MASK DesiredAccess,
                IN USHORT ShareAccess,
                IN ULONG CreateDisposition)
{
    IO_STATUS_BLOCK Iosb;
    PFCB Dcb;
    NTSTATUS Status;
    PCCB Ccb;

    /* Reference our DCB */
    Dcb = Vcb->RootDcb;

    DPRINT("Opening root directory\n");

    /* Exclusively lock this DCB */
    (VOID)FatAcquireExclusiveFcb(IrpContext, Dcb);

    do
    {
        /* Validate parameters */
        if (CreateDisposition != FILE_OPEN &&
            CreateDisposition != FILE_OPEN_IF)
        {
            Iosb.Status = STATUS_ACCESS_DENIED;
            break;
        }

        // TODO: Check file access

        /* Is it a first time open? */
        if (Dcb->OpenCount == 0)
        {
            /* Set share access */
            IoSetShareAccess(*DesiredAccess,
                             ShareAccess,
                             FileObject,
                             &Dcb->ShareAccess);
        }
        else
        {
            /* Check share access */
            Status = IoCheckShareAccess(*DesiredAccess,
                                        ShareAccess,
                                        FileObject,
                                        &Dcb->ShareAccess,
                                        TRUE);
        }

        /* Set file object pointers */
        Ccb = FatCreateCcb();
        FatSetFileObject(FileObject, UserDirectoryOpen, Dcb, Ccb);

        /* Increment counters */
        Dcb->OpenCount++;
        Dcb->UncleanCount++;
        Vcb->OpenFileCount++;
        if (IsFileObjectReadOnly(FileObject)) Vcb->ReadOnlyCount++;

        /* Set success statuses */
        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;
    } while (FALSE);

    /* Release the DCB lock */
    FatReleaseFcb(IrpContext, Dcb);

    return Iosb;
}

FF_ERROR
NTAPI
FatiTryToOpen(IN PFILE_OBJECT FileObject,
              IN PVCB Vcb)
{
    OEM_STRING AnsiName;
    CHAR AnsiNameBuf[512];
    FF_ERROR Error;
    NTSTATUS Status;
    FF_FILE *FileHandle;

    /* Convert the name to ANSI */
    AnsiName.Buffer = AnsiNameBuf;
    AnsiName.Length = 0;
    AnsiName.MaximumLength = sizeof(AnsiNameBuf);
    RtlZeroMemory(AnsiNameBuf, sizeof(AnsiNameBuf));
    Status = RtlUpcaseUnicodeStringToCountedOemString(&AnsiName, &FileObject->FileName, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    /* Open the file with FullFAT */
    FileHandle = FF_Open(Vcb->Ioman, AnsiName.Buffer, FF_MODE_READ, &Error);

    /* Close the handle */
    if (FileHandle) FF_Close(FileHandle);

    /* Return status */
    return Error;
}

IO_STATUS_BLOCK
NTAPI
FatiOverwriteFile(PFAT_IRP_CONTEXT IrpContext,
                  PFILE_OBJECT FileObject,
                  PFCB Fcb,
                  ULONG AllocationSize,
                  PFILE_FULL_EA_INFORMATION EaBuffer,
                  ULONG EaLength,
                  UCHAR FileAttributes,
                  ULONG CreateDisposition,
                  BOOLEAN NoEaKnowledge)
{
    IO_STATUS_BLOCK Iosb = {{0}};
    PCCB Ccb;
    LARGE_INTEGER Zero;
    ULONG NotifyFilter;

    Zero.QuadPart = 0;

    /* Check Ea mismatch first */
    if (NoEaKnowledge && EaLength > 0)
    {
        Iosb.Status = STATUS_ACCESS_DENIED;
        return Iosb;
    }

    do
    {
        /* Check if it's not still mapped */
        if (!MmCanFileBeTruncated(&Fcb->SectionObjectPointers,
                                  &Zero))
        {
            /* Fail */
            Iosb.Status = STATUS_USER_MAPPED_FILE;
            break;
        }

        /* Set file object pointers */
        Ccb = FatCreateCcb();
        FatSetFileObject(FileObject,
                         UserFileOpen,
                         Fcb,
                         Ccb);

        FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

        /* Indicate that create is in progress */
        Fcb->Vcb->State |= VCB_STATE_CREATE_IN_PROGRESS;

        /* Purge the cache section */
        CcPurgeCacheSection(&Fcb->SectionObjectPointers, NULL, 0, FALSE);

        /* Add Eas */
        if (EaLength > 0)
        {
            ASSERT(FALSE);
        }

        /* Acquire the paging resource */
        (VOID)ExAcquireResourceExclusiveLite(Fcb->Header.PagingIoResource, TRUE);

        /* Initialize FCB header */
        Fcb->Header.FileSize.QuadPart = 0;
        Fcb->Header.ValidDataLength.QuadPart = 0;

        /* Let CC know about changed file size */
        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);

        // TODO: Actually truncate the file
        DPRINT1("TODO: Actually truncate file '%wZ' with a fullfat handle %x\n", &Fcb->FullFileName, Fcb->FatHandle);

        /* Release the paging resource */
        ExReleaseResourceLite(Fcb->Header.PagingIoResource);

        /* Specify truncate on close */
        Fcb->State |= FCB_STATE_TRUNCATE_ON_CLOSE;

        // TODO: Delete previous EA if needed

        /* Send notification about changes */
        NotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE |
                       FILE_NOTIFY_CHANGE_ATTRIBUTES |
                       FILE_NOTIFY_CHANGE_SIZE;

        FsRtlNotifyFullReportChange(Fcb->Vcb->NotifySync,
                                    &Fcb->Vcb->NotifyList,
                                    (PSTRING)&Fcb->FullFileName,
                                    Fcb->FullFileName.Length - Fcb->FileNameLength,
                                    NULL,
                                    NULL,
                                    NotifyFilter,
                                    FILE_ACTION_MODIFIED,
                                    NULL);

        /* Set success status */
        Iosb.Status = STATUS_SUCCESS;

        /* Set correct information code */
        Iosb.Information = (CreateDisposition == FILE_SUPERSEDE) ? FILE_SUPERSEDED : FILE_OVERWRITTEN;
    } while (0);

    /* Remove the create in progress flag */
    ClearFlag(Fcb->Vcb->State, VCB_STATE_CREATE_IN_PROGRESS);

    return Iosb;
}

IO_STATUS_BLOCK
NTAPI
FatiOpenExistingDir(IN PFAT_IRP_CONTEXT IrpContext,
                     IN PFILE_OBJECT FileObject,
                     IN PVCB Vcb,
                     IN PFCB ParentDcb,
                     IN PACCESS_MASK DesiredAccess,
                     IN USHORT ShareAccess,
                     IN ULONG AllocationSize,
                     IN PFILE_FULL_EA_INFORMATION EaBuffer,
                     IN ULONG EaLength,
                     IN UCHAR FileAttributes,
                     IN ULONG CreateDisposition,
                     IN BOOLEAN DeleteOnClose)
{
    IO_STATUS_BLOCK Iosb = {{0}};
    OEM_STRING AnsiName;
    CHAR AnsiNameBuf[512];
    PFCB Fcb;
    NTSTATUS Status;
    FF_FILE *FileHandle;

    /* Only open is permitted */
    if (CreateDisposition != FILE_OPEN &&
        CreateDisposition != FILE_OPEN_IF)
    {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;
        return Iosb;
    }

    // TODO: Check dir access

    /* Convert the name to ANSI */
    AnsiName.Buffer = AnsiNameBuf;
    AnsiName.Length = 0;
    AnsiName.MaximumLength = sizeof(AnsiNameBuf);
    RtlZeroMemory(AnsiNameBuf, sizeof(AnsiNameBuf));
    Status = RtlUpcaseUnicodeStringToCountedOemString(&AnsiName, &FileObject->FileName, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    /* Open the dir with FullFAT */
    FileHandle = FF_Open(Vcb->Ioman, AnsiName.Buffer, FF_MODE_DIR, NULL);

    if (!FileHandle)
    {
        Iosb.Status = STATUS_OBJECT_NAME_NOT_FOUND; // FIXME: A shortcut for now
        return Iosb;
    }

    /* Create a new DCB for this directory */
    Fcb = FatCreateDcb(IrpContext, Vcb, ParentDcb, FileHandle);

    /* Set share access */
    IoSetShareAccess(*DesiredAccess, ShareAccess, FileObject, &Fcb->ShareAccess);

    /* Set context and section object pointers */
    FatSetFileObject(FileObject,
                     UserDirectoryOpen,
                     Fcb,
                     FatCreateCcb());

    /* Increase counters */
    Fcb->UncleanCount++;
    Fcb->OpenCount++;
    Vcb->OpenFileCount++;
    if (IsFileObjectReadOnly(FileObject)) Vcb->ReadOnlyCount++;

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    DPRINT1("Successfully opened dir %s\n", AnsiNameBuf);

    return Iosb;
}

IO_STATUS_BLOCK
NTAPI
FatiOpenExistingFile(IN PFAT_IRP_CONTEXT IrpContext,
                     IN PFILE_OBJECT FileObject,
                     IN PVCB Vcb,
                     IN PFCB ParentDcb,
                     IN PACCESS_MASK DesiredAccess,
                     IN USHORT ShareAccess,
                     IN ULONG AllocationSize,
                     IN PFILE_FULL_EA_INFORMATION EaBuffer,
                     IN ULONG EaLength,
                     IN UCHAR FileAttributes,
                     IN ULONG CreateDisposition,
                     IN BOOLEAN IsPagingFile,
                     IN BOOLEAN DeleteOnClose,
                     IN BOOLEAN IsDosName)
{
    IO_STATUS_BLOCK Iosb = {{0}};
    OEM_STRING AnsiName;
    CHAR AnsiNameBuf[512];
    PFCB Fcb;
    NTSTATUS Status;
    FF_FILE *FileHandle;
    FF_ERROR FfError;

    /* Check for create file option and fail */
    if (CreateDisposition == FILE_CREATE)
    {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;
        return Iosb;
    }

    // TODO: Check more params

    /* Convert the name to ANSI */
    AnsiName.Buffer = AnsiNameBuf;
    AnsiName.Length = 0;
    AnsiName.MaximumLength = sizeof(AnsiNameBuf);
    RtlZeroMemory(AnsiNameBuf, sizeof(AnsiNameBuf));
    Status = RtlUpcaseUnicodeStringToCountedOemString(&AnsiName, &FileObject->FileName, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    /* Open the file with FullFAT */
    FileHandle = FF_Open(Vcb->Ioman, AnsiName.Buffer, FF_MODE_READ, &FfError);

    if (!FileHandle)
    {
        DPRINT1("Failed to open file '%s', error %ld\n", AnsiName.Buffer, FfError);
        Iosb.Status = STATUS_OBJECT_NAME_NOT_FOUND; // FIXME: A shortcut for now
        return Iosb;
    }
    DPRINT1("Succeeded opening file '%s'\n", AnsiName.Buffer);

    /* Create a new FCB for this file */
    Fcb = FatCreateFcb(IrpContext, Vcb, ParentDcb, FileHandle);

    // TODO: Check if overwrite is needed

    // TODO: This is usual file open branch, without overwriting!
    /* Set context and section object pointers */
    FatSetFileObject(FileObject,
                     UserFileOpen,
                     Fcb,
                     FatCreateCcb());
    FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;


    /* Increase counters */
    Fcb->UncleanCount++;
    Fcb->OpenCount++;
    if (FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING)) Fcb->NonCachedUncleanCount++;
    if (IsFileObjectReadOnly(FileObject)) Vcb->ReadOnlyCount++;

    return Iosb;
}

IO_STATUS_BLOCK
NTAPI
FatiOpenVolume(IN PFAT_IRP_CONTEXT IrpContext,
               IN PFILE_OBJECT FileObject,
               IN PVCB Vcb,
               IN PACCESS_MASK DesiredAccess,
               IN USHORT ShareAccess,
               IN ULONG CreateDisposition)
{
    PCCB Ccb;
    IO_STATUS_BLOCK Iosb = {{0}};
    BOOLEAN VolumeFlushed = FALSE;

    /* Check parameters */
    if (CreateDisposition != FILE_OPEN &&
        CreateDisposition != FILE_OPEN_IF)
    {
        /* Deny access */
        Iosb.Status = STATUS_ACCESS_DENIED;
    }

    /* Check if it's exclusive open */
    if (!FlagOn(ShareAccess, FILE_SHARE_WRITE) &&
        !FlagOn(ShareAccess, FILE_SHARE_DELETE))
    {
        // TODO: Check if exclusive read access requested
        // and opened handles count is not 0
        //if (!FlagOn(ShareAccess, FILE_SHARE_READ)

        DPRINT1("Exclusive volume open\n");

        // TODO: Flush the volume
        VolumeFlushed = TRUE;
    }
    else if (FlagOn(*DesiredAccess, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA))
    {
        DPRINT1("Shared open\n");

        // TODO: Flush the volume
        VolumeFlushed = TRUE;
    }

    if (VolumeFlushed &&
        !FlagOn(Vcb->State, VCB_STATE_MOUNTED_DIRTY) &&
        FlagOn(Vcb->State, VCB_STATE_FLAG_DIRTY) &&
        CcIsThereDirtyData(Vcb->Vpb))
    {
        UNIMPLEMENTED;
    }

    /* Set share access */
    if (Vcb->DirectOpenCount > 0)
    {
        /* This volume has already been opened */
        Iosb.Status = IoCheckShareAccess(*DesiredAccess,
                                         ShareAccess,
                                         FileObject,
                                         &Vcb->ShareAccess,
                                         TRUE);

        if (!NT_SUCCESS(Iosb.Status))
        {
            ASSERT(FALSE);
        }
    }
    else
    {
        /* This is the first time open */
        IoSetShareAccess(*DesiredAccess,
                         ShareAccess,
                         FileObject,
                         &Vcb->ShareAccess);
    }

    /* Set file object pointers */
    Ccb = FatCreateCcb();
    FatSetFileObject(FileObject, UserVolumeOpen, Vcb, Ccb);
    FileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;

    /* Increase direct open count */
    Vcb->DirectOpenCount++;
    Vcb->OpenFileCount++;
    if (IsFileObjectReadOnly(FileObject)) Vcb->ReadOnlyCount++;

    /* Set no buffering flag */
    FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;

    // TODO: User's access check

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    return Iosb;
}

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
    BOOLEAN IsDirectoryFile;
    BOOLEAN NonDirectoryFile;
    BOOLEAN NoEaKnowledge;
    BOOLEAN DeleteOnClose;
    BOOLEAN TemporaryFile;
    ULONG CreateDisposition;

    /* Control blocks */
    PVCB Vcb, DecodedVcb, RelatedVcb;
    PFCB Fcb, NextFcb, RelatedDcb;
    PCCB Ccb, RelatedCcb;
    PFCB ParentDcb;

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
    IO_STATUS_BLOCK Iosb;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN EndBackslash = FALSE, OpenedAsDos, FirstRun = TRUE;
    UNICODE_STRING RemainingPart, FirstName, NextName, FileNameUpcased;
    OEM_STRING AnsiFirstName;
    FF_ERROR FfError;
    TYPE_OF_OPEN TypeOfOpen;
    BOOLEAN OplockPostIrp = FALSE;

    Iosb.Status = STATUS_SUCCESS;

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("FatCommonCreate\n", 0 );
    DPRINT("Irp                       = %08lx\n",   Irp );
    DPRINT("\t->Flags                   = %08lx\n", Irp->Flags );
    DPRINT("\t->FileObject              = %08lx\n", IrpSp->FileObject );
    DPRINT("\t->RelatedFileObject       = %08lx\n", IrpSp->FileObject->RelatedFileObject );
    DPRINT("\t->FileName                = %wZ\n",   &IrpSp->FileObject->FileName );
    DPRINT("\t->AllocationSize.LowPart  = %08lx\n", Irp->Overlay.AllocationSize.LowPart );
    DPRINT("\t->AllocationSize.HighPart = %08lx\n", Irp->Overlay.AllocationSize.HighPart );
    DPRINT("\t->SystemBuffer            = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DPRINT("\t->DesiredAccess           = %08lx\n", IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
    DPRINT("\t->Options                 = %08lx\n", IrpSp->Parameters.Create.Options );
    DPRINT("\t->FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DPRINT("\t->ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DPRINT("\t->EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength );

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

    /* Reject open by id */
    if (Options & FILE_OPEN_BY_FILE_ID)
    {
        FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    /* Prepare file attributes mask */
    FileAttributes &= (FILE_ATTRIBUTE_READONLY |
                       FILE_ATTRIBUTE_HIDDEN   |
                       FILE_ATTRIBUTE_SYSTEM   |
                       FILE_ATTRIBUTE_ARCHIVE);

    /* Get the volume control object */
    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    /* Get options */
    IsDirectoryFile         = BooleanFlagOn(Options, FILE_DIRECTORY_FILE);
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
    CreateDirectory = (BOOLEAN)(IsDirectoryFile &&
                                ((CreateDisposition == FILE_CREATE) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    OpenDirectory   = (BOOLEAN)(IsDirectoryFile &&
                                ((CreateDisposition == FILE_OPEN) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    /* Validate parameters: directory/nondirectory mismatch and
       AllocationSize being more than 4GB */
    if ((IsDirectoryFile && NonDirectoryFile) ||
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

        /* Set volume dismount status */
        if (Vcb->Condition != VcbGood)
            Status = STATUS_VOLUME_DISMOUNTED;

        /* Cleanup and return */
        FatReleaseVcb(IrpContext, Vcb);
        FatCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    /* Check if the volume is write protected and disallow DELETE_ON_CLOSE */
    if (DeleteOnClose & FlagOn(Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED))
    {
        ASSERT(FALSE);
        return STATUS_NOT_IMPLEMENTED;
    }

    // TODO: Make sure EAs aren't supported on FAT32

    /* Check if it's a volume open request */
    if (FileName.Length == 0)
    {
        /* Test related FO to be sure */
        if (!RelatedFO ||
            FatDecodeFileObject(RelatedFO, &DecodedVcb, &Fcb, &Ccb) == UserVolumeOpen)
        {
            /* Check parameters */
            if (IsDirectoryFile || OpenTargetDirectory)
            {
                Status = IsDirectoryFile ? STATUS_NOT_A_DIRECTORY : STATUS_INVALID_PARAMETER;

                /* Unlock VCB */
                FatReleaseVcb(IrpContext, Vcb);

                /* Complete the request and return */
                FatCompleteRequest(IrpContext, Irp, Status);
                return Status;
            }

            /* It is indeed a volume open request */
            Iosb = FatiOpenVolume(IrpContext,
                                  FileObject,
                                  Vcb,
                                  DesiredAccess,
                                  ShareAccess,
                                  CreateDisposition);

            /* Set resulting information */
            Irp->IoStatus.Information = Iosb.Information;

            /* Unlock VCB */
            FatReleaseVcb(IrpContext, Vcb);

            /* Complete the request and return */
            FatCompleteRequest(IrpContext, Irp, Iosb.Status);
            return Iosb.Status;
        }
    }

    /* Check if this is a relative open */
    if (RelatedFO)
    {
        /* Decode the file object */
        TypeOfOpen = FatDecodeFileObject(RelatedFO,
                                         &RelatedVcb,
                                         &RelatedDcb,
                                         &RelatedCcb);

        /* Check open type */
        if (TypeOfOpen != UserFileOpen &&
            TypeOfOpen != UserDirectoryOpen)
        {
            DPRINT1("Invalid file object!\n");

            Status = STATUS_OBJECT_PATH_NOT_FOUND;

            /* Cleanup and return */
            FatReleaseVcb(IrpContext, Vcb);
            FatCompleteRequest(IrpContext, Irp, Status);
            return Status;
        }

        /* File path must be relative */
        if (FileName.Length != 0 &&
            FileName.Buffer[0] == L'\\')
        {
            Status = STATUS_OBJECT_NAME_INVALID;

            /* The name is absolute, fail */
            FatReleaseVcb(IrpContext, Vcb);
            FatCompleteRequest(IrpContext, Irp, Status);
            return Status;
        }

        /* Make sure volume is the same */
        ASSERT(RelatedVcb == Vcb);

        /* Save VPB */
        FileObject->Vpb = RelatedFO->Vpb;

        /* Set parent DCB */
        ParentDcb = RelatedDcb;

        DPRINT("Opening file '%wZ' relatively to '%wZ'\n", &FileName, &ParentDcb->FullFileName);
    }
    else
    {
        /* Absolute open */
        if ((FileName.Length == sizeof(WCHAR)) &&
            (FileName.Buffer[0] == L'\\'))
        {
            /* Check if it's ok to open it */
            if (NonDirectoryFile)
            {
                DPRINT1("Trying to open root dir as a file\n");
                Status = STATUS_FILE_IS_A_DIRECTORY;

                /* Cleanup and return */
                FatReleaseVcb(IrpContext, Vcb);
                FatCompleteRequest(IrpContext, Irp, Status);
                return Status;
            }

            /* Check for target directory on a root dir */
            if (OpenTargetDirectory)
            {
                Status = STATUS_INVALID_PARAMETER;

                /* Cleanup and return */
                FatReleaseVcb(IrpContext, Vcb);
                FatCompleteRequest(IrpContext, Irp, Status);
                return Status;
            }

            /* Check delete on close on a root dir */
            if (DeleteOnClose)
            {
                Status = STATUS_CANNOT_DELETE;

                /* Cleanup and return */
                FatReleaseVcb(IrpContext, Vcb);
                FatCompleteRequest(IrpContext, Irp, Status);
                return Status;
            }

            /* Call root directory open routine */
            Iosb = FatiOpenRootDcb(IrpContext,
                                   FileObject,
                                   Vcb,
                                   DesiredAccess,
                                   ShareAccess,
                                   CreateDisposition);

            Irp->IoStatus.Information = Iosb.Information;

            /* Cleanup and return */
            FatReleaseVcb(IrpContext, Vcb);
            FatCompleteRequest(IrpContext, Irp, Iosb.Status);
            return Iosb.Status;
        }
        else
        {
            /* Not a root dir */
            ParentDcb = Vcb->RootDcb;
            DPRINT("ParentDcb %p\n", ParentDcb);
        }
    }

    /* Check for backslash at the end */
    if (FileName.Length &&
        FileName.Buffer[FileName.Length / sizeof(WCHAR) - 1] == L'\\')
    {
        /* Cut it out */
        FileName.Length -= sizeof(WCHAR);

        /* Remember we cut it */
        EndBackslash = TRUE;
    }

    /* Ensure the name is set */
    if (!ParentDcb->FullFileName.Buffer)
    {
        /* Set it if it's missing */
        FatSetFullFileNameInFcb(IrpContext, ParentDcb);
    }

    /* Check max path length */
    if (ParentDcb->FullFileName.Length + FileName.Length + sizeof(WCHAR) <= FileName.Length)
    {
        DPRINT1("Max length is way off\n");
        Iosb.Status = STATUS_OBJECT_NAME_INVALID;
        ASSERT(FALSE);
    }

    /* Loop through FCBs to find a good one */
    while (TRUE)
    {
        Fcb = ParentDcb;

        /* Dissect the name */
        RemainingPart = FileName;
        while (RemainingPart.Length)
        {
            FsRtlDissectName(RemainingPart, &FirstName, &NextName);

            /* Check for validity */
            if ((NextName.Length && NextName.Buffer[0] == L'\\') ||
                (NextName.Length > 255 * sizeof(WCHAR)))
            {
                /* The name is invalid */
                DPRINT1("Invalid name found\n");
                Iosb.Status = STATUS_OBJECT_NAME_INVALID;
                ASSERT(FALSE);
            }

            /* Convert the name to ANSI */
            AnsiFirstName.Buffer = ExAllocatePool(PagedPool, FirstName.Length);
            AnsiFirstName.Length = 0;
            AnsiFirstName.MaximumLength = FirstName.Length;
            Status = RtlUpcaseUnicodeStringToCountedOemString(&AnsiFirstName, &FirstName, FALSE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("RtlUpcaseUnicodeStringToCountedOemString() failed with 0x%08x\n", Status);
                ASSERT(FALSE);
                NextFcb = NULL;
                AnsiFirstName.Length = 0;
            }
            else
            {
                /* Find the coresponding FCB */
                NextFcb = FatFindFcb(IrpContext,
                                     &Fcb->Dcb.SplayLinksAnsi,
                                     (PSTRING)&AnsiFirstName,
                                     &OpenedAsDos);
            }

            /* If nothing found - try with unicode */
            if (!NextFcb && Fcb->Dcb.SplayLinksUnicode)
            {
                FileNameUpcased.Buffer = FsRtlAllocatePool(PagedPool, FirstName.Length);
                FileNameUpcased.Length = 0;
                FileNameUpcased.MaximumLength = FirstName.Length;

                /* Downcase and then upcase to normalize it */
                Status = RtlDowncaseUnicodeString(&FileNameUpcased, &FirstName, FALSE);
                Status = RtlUpcaseUnicodeString(&FileNameUpcased, &FileNameUpcased, FALSE);

                /* Try to find FCB again using unicode name */
                NextFcb = FatFindFcb(IrpContext,
                                     &Fcb->Dcb.SplayLinksUnicode,
                                     (PSTRING)&FileNameUpcased,
                                     &OpenedAsDos);
            }

            /* Move to the next FCB */
            if (NextFcb)
            {
                Fcb = NextFcb;
                RemainingPart = NextName;
            }

            /* Break out of this loop if nothing can be found */
            if (!NextFcb ||
                NextName.Length == 0 ||
                FatNodeType(NextFcb) == FAT_NTC_FCB)
            {
                break;
            }
        }

        /* Ensure remaining name doesn't start from a backslash */
        if (RemainingPart.Length &&
            RemainingPart.Buffer[0] == L'\\')
        {
            /* Cut it */
            RemainingPart.Buffer++;
            RemainingPart.Length -= sizeof(WCHAR);
        }

        if (Fcb->Condition == FcbGood)
        {
            /* Good FCB, break out of the loop */
            break;
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    /* Treat page file in a special way */
    if (IsPagingFile)
    {
        UNIMPLEMENTED;
        // FIXME: System file too
    }

    /* Make sure there is no pending delete on a higher-level FCB */
    if (Fcb->State & FCB_STATE_DELETE_ON_CLOSE)
    {
        Iosb.Status = STATUS_DELETE_PENDING;

        /* Cleanup and return */
        FatReleaseVcb(IrpContext, Vcb);

        /* Complete the request */
        FatCompleteRequest(IrpContext, Irp, Iosb.Status);

        return Iosb.Status;
    }

    /* We have a valid FCB now */
    if (!RemainingPart.Length)
    {
        /* Check for target dir open */
        if (OpenTargetDirectory)
        {
            DPRINT1("Opening target dir is missing\n");
            ASSERT(FALSE);
        }

        /* Check this FCB's type */
        if (FatNodeType(Fcb) == FAT_NTC_ROOT_DCB ||
            FatNodeType(Fcb) == FAT_NTC_DCB)
        {
            /* Open a directory */
            if (NonDirectoryFile)
            {
                /* Forbidden */
                Iosb.Status = STATUS_FILE_IS_A_DIRECTORY;
                ASSERT(FALSE);
                return Iosb.Status;
            }

            /* Open existing DCB */
            Iosb = FatiOpenExistingDcb(IrpContext,
                                       FileObject,
                                       Vcb,
                                       Fcb,
                                       DesiredAccess,
                                       ShareAccess,
                                       CreateDisposition,
                                       NoEaKnowledge,
                                       DeleteOnClose);

            /* Save information */
            Irp->IoStatus.Information = Iosb.Information;

            /* Unlock VCB */
            FatReleaseVcb(IrpContext, Vcb);

            /* Complete the request */
            FatCompleteRequest(IrpContext, Irp, Iosb.Status);

            return Iosb.Status;
        }
        else if (FatNodeType(Fcb) == FAT_NTC_FCB)
        {
            /* Open a file */
            if (OpenDirectory)
            {
                /* Forbidden */
                Iosb.Status = STATUS_NOT_A_DIRECTORY;
                ASSERT(FALSE);
                return Iosb.Status;
            }

            /* Check for trailing backslash */
            if (EndBackslash)
            {
                /* Forbidden */
                Iosb.Status = STATUS_OBJECT_NAME_INVALID;
                ASSERT(FALSE);
                return Iosb.Status;
            }

            Iosb = FatiOpenExistingFcb(IrpContext,
                                       FileObject,
                                       Vcb,
                                       Fcb,
                                       DesiredAccess,
                                       ShareAccess,
                                       AllocationSize,
                                       EaBuffer,
                                       EaLength,
                                       FileAttributes,
                                       CreateDisposition,
                                       NoEaKnowledge,
                                       DeleteOnClose,
                                       OpenedAsDos,
                                       &OplockPostIrp);

            /* Check if it's pending */
            if (Iosb.Status != STATUS_PENDING)
            {
                /* In case of success set cache supported flag */
                if (NT_SUCCESS(Iosb.Status) && !NoIntermediateBuffering)
                {
                    SetFlag(FileObject->Flags, FO_CACHE_SUPPORTED);
                }

                /* Save information */
                Irp->IoStatus.Information = Iosb.Information;

                /* Unlock VCB */
                FatReleaseVcb(IrpContext, Vcb);

                /* Complete the request */
                FatCompleteRequest(IrpContext, Irp, Iosb.Status);

                return Iosb.Status;
            }
            else
            {
                /* Queue this IRP */
                UNIMPLEMENTED;
                ASSERT(FALSE);
            }
        }
        else
        {
            /* Unexpected FCB type */
            KeBugCheckEx(FAT_FILE_SYSTEM, __LINE__, (ULONG_PTR)Fcb, 0, 0);
        }
    }

    /* During parsing we encountered a part which has no attached FCB/DCB.
    Check that the parent is really DCB and not FCB */
    if (FatNodeType(Fcb) != FAT_NTC_ROOT_DCB &&
        FatNodeType(Fcb) != FAT_NTC_DCB)
    {
        DPRINT1("Weird FCB node type %x, expected DCB or root DCB\n", FatNodeType(Fcb));
        ASSERT(FALSE);
    }

    /* Create additional DCBs for all path items */
    ParentDcb = Fcb;
    while (TRUE)
    {
        if (FirstRun)
        {
            RemainingPart = NextName;
            if (AnsiFirstName.Length)
                Status = STATUS_SUCCESS;
            else
                Status = STATUS_UNMAPPABLE_CHARACTER;

            /* First run init is done */
            FirstRun = FALSE;
        }
        else
        {
            FsRtlDissectName(RemainingPart, &FirstName, &RemainingPart);

            /* Check for validity */
            if ((RemainingPart.Length && RemainingPart.Buffer[0] == L'\\') ||
                (NextName.Length > 255 * sizeof(WCHAR)))
            {
                /* The name is invalid */
                DPRINT1("Invalid name found\n");
                Iosb.Status = STATUS_OBJECT_NAME_INVALID;
                ASSERT(FALSE);
            }

            /* Convert the name to ANSI */
            AnsiFirstName.Buffer = ExAllocatePool(PagedPool, FirstName.Length);
            AnsiFirstName.Length = 0;
            AnsiFirstName.MaximumLength = FirstName.Length;
            Status = RtlUpcaseUnicodeStringToCountedOemString(&AnsiFirstName, &FirstName, FALSE);
        }

        if (!NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
        }

        DPRINT("FirstName %wZ, RemainingPart %wZ\n", &FirstName, &RemainingPart);

        /* Break if came to the end */
        if (!RemainingPart.Length) break;

        /* Create a DCB for this entry */
        ParentDcb = FatCreateDcb(IrpContext,
                                 Vcb,
                                 ParentDcb,
                                 NULL);

        /* Set its name */
        FatSetFullNameInFcb(ParentDcb, &FirstName);
    }

    /* Try to open it and get a result, saying if this is a dir or a file */
    FfError = FatiTryToOpen(FileObject, Vcb);

    /* Check if we need to open target directory */
    if (OpenTargetDirectory)
    {
        // TODO: Open target directory
        UNIMPLEMENTED;
    }

    /* Check, if path is a existing directory or file */
    if (FfError == FF_ERR_FILE_OBJECT_IS_A_DIR ||
        FfError == FF_ERR_FILE_ALREADY_OPEN ||
        FfError == FF_ERR_NONE)
    {
        if (FfError == FF_ERR_FILE_OBJECT_IS_A_DIR)
        {
            if (NonDirectoryFile)
            {
                DPRINT1("Can't open dir as a file\n");

                /* Unlock VCB */
                FatReleaseVcb(IrpContext, Vcb);

                /* Complete the request */
                Iosb.Status = STATUS_FILE_IS_A_DIRECTORY;
                FatCompleteRequest(IrpContext, Irp, Iosb.Status);
                return Iosb.Status;
            }

            /* Open this directory */
            Iosb = FatiOpenExistingDir(IrpContext,
                FileObject,
                Vcb,
                ParentDcb,
                DesiredAccess,
                ShareAccess,
                AllocationSize,
                EaBuffer,
                EaLength,
                FileAttributes,
                CreateDisposition,
                DeleteOnClose);

            Irp->IoStatus.Information = Iosb.Information;

            /* Unlock VCB */
            FatReleaseVcb(IrpContext, Vcb);

            /* Complete the request */
            FatCompleteRequest(IrpContext, Irp, Iosb.Status);

            return Iosb.Status;
        }
        else
        {
            /* This is opening an existing file */
            if (OpenDirectory)
            {
                /* But caller wanted a dir */
                Status = STATUS_NOT_A_DIRECTORY;

                /* Unlock VCB */
                FatReleaseVcb(IrpContext, Vcb);

                /* Complete the request */
                FatCompleteRequest(IrpContext, Irp, Status);

                return Status;
            }

            /* If end backslash here, then it's definately not permitted,
            since we're opening files here */
            if (EndBackslash)
            {
                /* Unlock VCB */
                FatReleaseVcb(IrpContext, Vcb);

                /* Complete the request */
                Iosb.Status = STATUS_OBJECT_NAME_INVALID;
                FatCompleteRequest(IrpContext, Irp, Iosb.Status);
                return Iosb.Status;
            }

            /* Try to open the file */
            Iosb = FatiOpenExistingFile(IrpContext,
                                        FileObject,
                                        Vcb,
                                        ParentDcb,
                                        DesiredAccess,
                                        ShareAccess,
                                        AllocationSize,
                                        EaBuffer,
                                        EaLength,
                                        FileAttributes,
                                        CreateDisposition,
                                        FALSE,
                                        DeleteOnClose,
                                        OpenedAsDos);

            /* In case of success set cache supported flag */
            if (NT_SUCCESS(Iosb.Status) && !NoIntermediateBuffering)
            {
                SetFlag(FileObject->Flags, FO_CACHE_SUPPORTED);
            }

            Irp->IoStatus.Information = Iosb.Information;

            /* Unlock VCB */
            FatReleaseVcb(IrpContext, Vcb);

            /* Complete the request */
            FatCompleteRequest(IrpContext, Irp, Iosb.Status);

            return Iosb.Status;
        }
    }

    /* We come here only in the case when a new file is created */
    //ASSERT(FALSE);
    DPRINT1("TODO: Create a new file/directory, called '%wZ'\n", &IrpSp->FileObject->FileName);

    Status = STATUS_NOT_IMPLEMENTED;

    /* Unlock VCB */
    FatReleaseVcb(IrpContext, Vcb);

    /* Complete the request */
    FatCompleteRequest(IrpContext, Irp, Status);

    return Status;
}

NTSTATUS
NTAPI
FatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFAT_IRP_CONTEXT IrpContext;
    NTSTATUS Status;

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

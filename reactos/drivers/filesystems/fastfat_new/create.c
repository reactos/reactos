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

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    IN OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);


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

    DPRINT1("Opening root directory\n");

    Iosb.Status = STATUS_NOT_IMPLEMENTED;

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
    Fcb = FatCreateDcb(IrpContext, Vcb, ParentDcb);
    Fcb->FatHandle = FileHandle;

    /* Set share access */
    IoSetShareAccess(*DesiredAccess, ShareAccess, FileObject, &Fcb->ShareAccess);

    /* Set context and section object pointers */
    FatSetFileObject(FileObject,
                     UserDirectoryOpen,
                     Fcb,
                     FatCreateCcb());

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
    FileHandle = FF_Open(Vcb->Ioman, AnsiName.Buffer, FF_MODE_READ, NULL);

    if (!FileHandle)
    {
        Iosb.Status = STATUS_OBJECT_NAME_NOT_FOUND; // FIXME: A shortcut for now
        return Iosb;
    }

    /* Create a new FCB for this file */
    Fcb = FatCreateFcb(IrpContext, Vcb, ParentDcb, FileHandle);

    // TODO: Check if overwrite is needed

    // This is usual file open branch, without overwriting!
    /* Set context and section object pointers */
    FatSetFileObject(FileObject,
                     UserFileOpen,
                     Fcb,
                     FatCreateCcb());
    FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

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

        DPRINT1("Exclusive voume open\n");

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
    Ccb = FatCreateCcb(IrpContext);
    FatSetFileObject(FileObject, UserVolumeOpen, Vcb, Ccb);
    FileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;

    /* Increase direct open count */
    Vcb->DirectOpenCount++;
    Vcb->OpenFileCount++;

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
    BOOLEAN DirectoryFile;
    BOOLEAN NonDirectoryFile;
    BOOLEAN NoEaKnowledge;
    BOOLEAN DeleteOnClose;
    BOOLEAN TemporaryFile;
    ULONG CreateDisposition;

    /* Control blocks */
    PVCB Vcb, DecodedVcb;
    PFCB Fcb, NextFcb;
    PCCB Ccb;
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
    BOOLEAN EndBackslash = FALSE, OpenedAsDos;
    UNICODE_STRING RemainingPart, FirstName, NextName;
    OEM_STRING AnsiFirstName;
    FF_ERROR FfError;

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

    /* Check if it's a volume open request */
    if (FileName.Length == 0)
    {
        /* Test related FO to be sure */
        if (!RelatedFO ||
            FatDecodeFileObject(RelatedFO, &DecodedVcb, &Fcb, &Ccb) == UserVolumeOpen)
        {
            /* Check parameters */
            if (DirectoryFile || OpenTargetDirectory)
            {
                Status = DirectoryFile ? STATUS_NOT_A_DIRECTORY : STATUS_INVALID_PARAMETER;

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
        // RelatedFO will be a parent directory
        UNIMPLEMENTED;
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

                /* Cleanup and return */
                FatReleaseVcb(IrpContext, Vcb);
                return STATUS_FILE_IS_A_DIRECTORY;
            }

            /* Check delete on close on a root dir */
            if (DeleteOnClose)
            {
                /* Cleanup and return */
                FatReleaseVcb(IrpContext, Vcb);
                return STATUS_CANNOT_DELETE;
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
            return Iosb.Status;
        }
        else
        {
            /* Not a root dir */
            ParentDcb = Vcb->RootDcb;
            DPRINT("ParentDcb %p\n", ParentDcb);
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

                /* Check if we found anything */
                if (!NextFcb && Fcb->Dcb.SplayLinksUnicode)
                {
                    ASSERT(FALSE);
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

        /* We have a valid FCB now */
        if (!RemainingPart.Length)
        {
            DPRINT1("It's possible to open an existing FCB\n");
            ASSERT(FALSE);
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
                                     ParentDcb);

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

        /* Check, if path is a directory or a file */
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

        Irp->IoStatus.Information = Iosb.Information;
    }

    /* Unlock VCB */
    FatReleaseVcb(IrpContext, Vcb);

    /* Complete the request */
    FatCompleteRequest(IrpContext, Irp, Iosb.Status);

    return Iosb.Status;
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

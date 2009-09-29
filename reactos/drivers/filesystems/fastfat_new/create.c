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

    Iosb.Status = STATUS_SUCCESS;

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

    /* Check if it's a volume open request */
    if (FileName.Length == 0)
    {
        /* Test related FO to be sure */
        if (!RelatedFO ||
            FatDecodeFileObject(RelatedFO, &DecodedVcb, &Fcb, &Ccb) == UserVolumeOpen)
        {
            /* It is indeed a volume open request */
            DPRINT1("Volume open request, not implemented now!\n");
            UNIMPLEMENTED;
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
            DPRINT1("ParentDcb %p\n", ParentDcb);
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
            DPRINT1("ParentDcb->FullFileName.Buffer is NULL\n");
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

            DPRINT1("FirstName %wZ, RemainingPart %wZ\n", &FirstName, &RemainingPart);

            /* Break if came to the end */
            if (!RemainingPart.Length) break;

            // TODO: Create a DCB for this entry
        }

        // Simulate that we opened the file
        //Iosb.Information = FILE_OPENED;
        Irp->IoStatus.Information = FILE_OPENED;
        FileObject->SectionObjectPointer = (PSECTION_OBJECT_POINTERS)0x1;
    }

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

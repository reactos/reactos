/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fileinfo.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"
#include <linux/ext4.h>

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2QueryFileInformation)
#pragma alloc_text(PAGE, Ext2SetFileInformation)
#pragma alloc_text(PAGE, Ext2ExpandFile)
#pragma alloc_text(PAGE, Ext2TruncateFile)
#pragma alloc_text(PAGE, Ext2SetDispositionInfo)
#pragma alloc_text(PAGE, Ext2SetRenameInfo)
#pragma alloc_text(PAGE, Ext2DeleteFile)
#endif

NTSTATUS
Ext2QueryFileInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT            FileObject;
    PEXT2_VCB               Vcb;
    PEXT2_FCB               Fcb;
    PEXT2_MCB               Mcb;
    PEXT2_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 FcbResourceAcquired = FALSE;

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

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        if (Fcb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (!((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)))) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        Vcb = Fcb->Vcb;

        {
            if (!ExAcquireResourceSharedLite(
                        &Fcb->MainResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)
                    )) {

                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }

            FcbResourceAcquired = TRUE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        ASSERT(Ccb != NULL);
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        Mcb = Ccb->SymLink;
        if (!Mcb)
            Mcb = Fcb->Mcb;

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        FileInformationClass =
            IoStackLocation->Parameters.QueryFile.FileInformationClass;

        Length = IoStackLocation->Parameters.QueryFile.Length;
        Buffer = Irp->AssociatedIrp.SystemBuffer;
        RtlZeroMemory(Buffer, Length);

        switch (FileInformationClass) {

        case FileBasicInformation:
        {
            PFILE_BASIC_INFORMATION FileBasicInformation;

            if (Length < sizeof(FILE_BASIC_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FileBasicInformation = (PFILE_BASIC_INFORMATION) Buffer;

            FileBasicInformation->CreationTime = Mcb->CreationTime;
            FileBasicInformation->LastAccessTime = Mcb->LastAccessTime;
            FileBasicInformation->LastWriteTime = Mcb->LastWriteTime;
            FileBasicInformation->ChangeTime = Mcb->ChangeTime;

            FileBasicInformation->FileAttributes = Mcb->FileAttr;
            if (IsLinkInvalid(Mcb)) {
                ClearFlag(FileBasicInformation->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
            }
            if (FileBasicInformation->FileAttributes == 0) {
                FileBasicInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }

            Irp->IoStatus.Information = sizeof(FILE_BASIC_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

        case FileStandardInformation:
        {
            PFILE_STANDARD_INFORMATION FSI;

            if (Length < sizeof(FILE_STANDARD_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FSI = (PFILE_STANDARD_INFORMATION) Buffer;

            FSI->NumberOfLinks = Mcb->Inode.i_nlink;

            if (IsVcbReadOnly(Fcb->Vcb))
                FSI->DeletePending = FALSE;
            else
                FSI->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);

            if (IsLinkInvalid(Mcb)) {
                FSI->Directory = FALSE;
                FSI->AllocationSize.QuadPart = 0;
                FSI->EndOfFile.QuadPart = 0;
            } else if (IsMcbDirectory(Mcb)) {
                FSI->Directory = TRUE;
                FSI->AllocationSize.QuadPart = 0;
                FSI->EndOfFile.QuadPart = 0;
            } else {
                FSI->Directory = FALSE;
                FSI->AllocationSize = Fcb->Header.AllocationSize;
                FSI->EndOfFile = Fcb->Header.FileSize;
            }

            Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

        case FileInternalInformation:
        {
            PFILE_INTERNAL_INFORMATION FileInternalInformation;

            if (Length < sizeof(FILE_INTERNAL_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FileInternalInformation = (PFILE_INTERNAL_INFORMATION) Buffer;

            /* we use the inode number as the internal index */
            FileInternalInformation->IndexNumber.QuadPart = (LONGLONG)Mcb->Inode.i_ino;

            Irp->IoStatus.Information = sizeof(FILE_INTERNAL_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;


        case FileEaInformation:
        {
            PFILE_EA_INFORMATION FileEaInformation;

            if (Length < sizeof(FILE_EA_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FileEaInformation = (PFILE_EA_INFORMATION) Buffer;

            // Romfs doesn't have any extended attributes
            FileEaInformation->EaSize = 0;

            Irp->IoStatus.Information = sizeof(FILE_EA_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

        case FileNameInformation:
        {
            PFILE_NAME_INFORMATION FileNameInformation;
            ULONG   BytesToCopy = 0;

            if (Length < (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) +
                    Mcb->FullName.Length) {
                BytesToCopy = Length - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
                Status = STATUS_BUFFER_OVERFLOW;
            } else {
                BytesToCopy = Mcb->FullName.Length;
                Status = STATUS_SUCCESS;
            }

            FileNameInformation = (PFILE_NAME_INFORMATION) Buffer;
            FileNameInformation->FileNameLength = Mcb->FullName.Length;

            RtlCopyMemory(
                FileNameInformation->FileName,
                Mcb->FullName.Buffer,
                BytesToCopy );

            Irp->IoStatus.Information = BytesToCopy +
                                        + FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
        }
        break;

        case FilePositionInformation:
        {
            PFILE_POSITION_INFORMATION FilePositionInformation;

            if (Length < sizeof(FILE_POSITION_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FilePositionInformation = (PFILE_POSITION_INFORMATION) Buffer;
            FilePositionInformation->CurrentByteOffset =
                FileObject->CurrentByteOffset;

            Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

        case FileAllInformation:
        {
            PFILE_ALL_INFORMATION       FileAllInformation;
            PFILE_BASIC_INFORMATION     FileBasicInformation;
            PFILE_STANDARD_INFORMATION  FSI;
            PFILE_INTERNAL_INFORMATION  FileInternalInformation;
            PFILE_EA_INFORMATION        FileEaInformation;
            PFILE_POSITION_INFORMATION  FilePositionInformation;
            PFILE_NAME_INFORMATION      FileNameInformation;

            if (Length < sizeof(FILE_ALL_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FileAllInformation = (PFILE_ALL_INFORMATION) Buffer;

            FileBasicInformation =
                &FileAllInformation->BasicInformation;

            FSI =
                &FileAllInformation->StandardInformation;

            FileInternalInformation =
                &FileAllInformation->InternalInformation;

            FileEaInformation =
                &FileAllInformation->EaInformation;

            FilePositionInformation =
                &FileAllInformation->PositionInformation;

            FileNameInformation =
                &FileAllInformation->NameInformation;

            FileBasicInformation->CreationTime = Mcb->CreationTime;
            FileBasicInformation->LastAccessTime = Mcb->LastAccessTime;
            FileBasicInformation->LastWriteTime = Mcb->LastWriteTime;
            FileBasicInformation->ChangeTime = Mcb->ChangeTime;

            FileBasicInformation->FileAttributes = Mcb->FileAttr;
            if (IsMcbSymLink(Mcb) && IsFileDeleted(Mcb->Target)) {
                ClearFlag(FileBasicInformation->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
            }
            if (FileBasicInformation->FileAttributes == 0) {
                FileBasicInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }

            FSI->NumberOfLinks = Mcb->Inode.i_nlink;

            if (IsVcbReadOnly(Fcb->Vcb))
                FSI->DeletePending = FALSE;
            else
                FSI->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);

            if (IsLinkInvalid(Mcb)) {
                FSI->Directory = FALSE;
                FSI->AllocationSize.QuadPart = 0;
                FSI->EndOfFile.QuadPart = 0;
            } else if (IsDirectory(Fcb)) {
                FSI->Directory = TRUE;
                FSI->AllocationSize.QuadPart = 0;
                FSI->EndOfFile.QuadPart = 0;
            } else {
                FSI->Directory = FALSE;
                FSI->AllocationSize = Fcb->Header.AllocationSize;
                FSI->EndOfFile = Fcb->Header.FileSize;
            }

            // The "inode number"
            FileInternalInformation->IndexNumber.QuadPart = (LONGLONG)Mcb->Inode.i_ino;

            // Romfs doesn't have any extended attributes
            FileEaInformation->EaSize = 0;

            FilePositionInformation->CurrentByteOffset =
                FileObject->CurrentByteOffset;

            FileNameInformation->FileNameLength = Mcb->ShortName.Length;

            if (Length < sizeof(FILE_ALL_INFORMATION) +
                    Mcb->ShortName.Length - sizeof(WCHAR)) {
                Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION);
                Status = STATUS_BUFFER_OVERFLOW;
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Mcb->ShortName.Buffer,
                    Length - FIELD_OFFSET(FILE_ALL_INFORMATION,
                                          NameInformation.FileName)
                );
                _SEH2_LEAVE;
            }

            RtlCopyMemory(
                FileNameInformation->FileName,
                Mcb->ShortName.Buffer,
                Mcb->ShortName.Length
            );

            Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION) +
                                        Mcb->ShortName.Length - sizeof(WCHAR);
#if 0
            sizeof(FILE_ACCESS_INFORMATION) -
            sizeof(FILE_MODE_INFORMATION) -
            sizeof(FILE_ALIGNMENT_INFORMATION);
#endif

            Status = STATUS_SUCCESS;
        }
        break;

        /*
        case FileAlternateNameInformation:
        {
            // TODO: Handle FileAlternateNameInformation

            // Here we would like to use RtlGenerate8dot3Name but I don't
            // know how to use the argument PGENERATE_NAME_CONTEXT
        }
        */

        case FileNetworkOpenInformation:
        {
            PFILE_NETWORK_OPEN_INFORMATION PFNOI;

            if (Length < sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            PFNOI = (PFILE_NETWORK_OPEN_INFORMATION) Buffer;

            PFNOI->FileAttributes = Mcb->FileAttr;
            if (IsLinkInvalid(Mcb)) {
                ClearFlag(PFNOI->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
                PFNOI->AllocationSize.QuadPart = 0;
                PFNOI->EndOfFile.QuadPart = 0;
            } else if (IsDirectory(Fcb)) {
                PFNOI->AllocationSize.QuadPart = 0;
                PFNOI->EndOfFile.QuadPart = 0;
            } else {
                PFNOI->AllocationSize = Fcb->Header.AllocationSize;
                PFNOI->EndOfFile      = Fcb->Header.FileSize;
            }

            if (PFNOI->FileAttributes == 0) {
                PFNOI->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }

            PFNOI->CreationTime   = Mcb->CreationTime;
            PFNOI->LastAccessTime = Mcb->LastAccessTime;
            PFNOI->LastWriteTime  = Mcb->LastWriteTime;
            PFNOI->ChangeTime     = Mcb->ChangeTime;


            Irp->IoStatus.Information =
                sizeof(FILE_NETWORK_OPEN_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

#if (_WIN32_WINNT >= 0x0500)

        case FileAttributeTagInformation:
        {
            PFILE_ATTRIBUTE_TAG_INFORMATION FATI;

            if (Length < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FATI = (PFILE_ATTRIBUTE_TAG_INFORMATION) Buffer;
            FATI->FileAttributes = Mcb->FileAttr;
            if (IsLinkInvalid(Mcb)) {
                ClearFlag(FATI->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
            }
            if (FATI->FileAttributes == 0) {
                FATI->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }
            FATI->ReparseTag = IO_REPARSE_TAG_RESERVED_ZERO;
            Irp->IoStatus.Information = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;
#endif // (_WIN32_WINNT >= 0x0500)

        case FileStreamInformation:
            Status = STATUS_INVALID_PARAMETER;
            break;

        default:
            DEBUG(DL_WRN, ( "Ext2QueryInformation: invalid class: %d\n",
                            FileInformationClass));
            Status = STATUS_INVALID_PARAMETER; /* STATUS_INVALID_INFO_CLASS; */
            break;
        }

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING ||
                    Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext,  Status);
            }
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2SetFileInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PFILE_OBJECT            FileObject;
    PEXT2_FCB               Fcb;
    PEXT2_CCB               Ccb;
    PEXT2_MCB               Mcb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;

    ULONG                   NotifyFilter = 0;

    ULONG                   Length;
    PVOID                   Buffer;

    BOOLEAN                 VcbMainResourceAcquired = FALSE;
    BOOLEAN                 FcbMainResourceAcquired = FALSE;
    BOOLEAN                 FcbPagingIoResourceAcquired = FALSE;

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

        /* check io stack location of irp stack */
        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        FileInformationClass =
            IoStackLocation->Parameters.SetFile.FileInformationClass;
        Length = IoStackLocation->Parameters.SetFile.Length;
        Buffer = Irp->AssociatedIrp.SystemBuffer;

        /* check Vcb */
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        if (!IsMounted(Vcb)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        /* we need grab Vcb in case it's a rename operation */
        if (FileInformationClass == FileRenameInformation) {
            if (!ExAcquireResourceExclusiveLite(
                        &Vcb->MainResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            VcbMainResourceAcquired = TRUE;
        }

        if (IsVcbReadOnly(Vcb)) {
            if (FileInformationClass != FilePositionInformation) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                _SEH2_LEAVE;
            }
        }

        if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            Status = STATUS_ACCESS_DENIED;
            _SEH2_LEAVE;
        }

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;

        // This request is issued to volumes, just return success
        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Mcb->Flags, MCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        ASSERT(Ccb != NULL);
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        Mcb = Ccb->SymLink;
        if (Mcb) {
            if (IsFlagOn(Mcb->Flags, MCB_FILE_DELETED)) {
                Status = STATUS_FILE_DELETED;
                _SEH2_LEAVE;
            }
        } else {
            Mcb = Fcb->Mcb;
        }

        if ( !IsDirectory(Fcb) && !FlagOn(Fcb->Flags, FCB_PAGE_FILE) &&
                ((FileInformationClass == FileEndOfFileInformation) ||
                 (FileInformationClass == FileValidDataLengthInformation) ||
                 (FileInformationClass == FileAllocationInformation))) {

            Status = FsRtlCheckOplock( &Fcb->Oplock,
                                       Irp,
                                       IrpContext,
                                       NULL,
                                       NULL );

            if (Status != STATUS_SUCCESS) {
                _SEH2_LEAVE;
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);
        }

        /* for renaming, we must not get any Fcb locks here, function
           Ext2SetRenameInfo will get Dcb resource exclusively.  */
        if (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE) &&
                FileInformationClass != FileRenameInformation) {

            if (!ExAcquireResourceExclusiveLite(
                        &Fcb->MainResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }

            FcbMainResourceAcquired = TRUE;

            if ( FileInformationClass == FileAllocationInformation ||
                 FileInformationClass == FileEndOfFileInformation ||
                 FileInformationClass == FileValidDataLengthInformation) {

                if (!ExAcquireResourceExclusiveLite(
                            &Fcb->PagingIoResource,
                            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                    Status = STATUS_PENDING;
                    DbgBreak();
                    _SEH2_LEAVE;
                }
                FcbPagingIoResourceAcquired = TRUE;
            }
        }

        switch (FileInformationClass) {

        case FileBasicInformation:
        {
            PFILE_BASIC_INFORMATION FBI = (PFILE_BASIC_INFORMATION) Buffer;
            struct inode *Inode = &Mcb->Inode;

            if (FBI->CreationTime.QuadPart != 0 && FBI->CreationTime.QuadPart != -1) {
                Inode->i_ctime = Ext2LinuxTime(FBI->CreationTime);
                Mcb->CreationTime = Ext2NtTime(Inode->i_ctime);
                NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
            }

            if (FBI->LastAccessTime.QuadPart != 0 && FBI->LastAccessTime.QuadPart != -1) {
                Inode->i_atime = Ext2LinuxTime(FBI->LastAccessTime);
                Mcb->LastAccessTime = Ext2NtTime(Inode->i_atime);
                NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
            }

            if (FBI->LastWriteTime.QuadPart != 0 && FBI->LastWriteTime.QuadPart != -1) {
                Inode->i_mtime = Ext2LinuxTime(FBI->LastWriteTime);
                Mcb->LastWriteTime = Ext2NtTime(Inode->i_mtime);
                NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
                SetFlag(Ccb->Flags, CCB_LAST_WRITE_UPDATED);
            }

            if (FBI->ChangeTime.QuadPart !=0 && FBI->ChangeTime.QuadPart != -1) {
                Mcb->ChangeTime = FBI->ChangeTime;
            }

            if (FBI->FileAttributes != 0) {

                BOOLEAN bIsDirectory = IsDirectory(Fcb);
                NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;

                if (IsFlagOn(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY)) {
                    Ext2SetOwnerReadOnly(Inode->i_mode);
                } else {
                    Ext2SetOwnerWritable(Inode->i_mode);
                }

                if (FBI->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                    SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                } else {
                    ClearFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                }

                Mcb->FileAttr = FBI->FileAttributes;
                if (bIsDirectory) {
                    SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                    ClearFlag(Mcb->FileAttr, FILE_ATTRIBUTE_NORMAL);
                }
            }

            if (NotifyFilter != 0) {
                if (Ext2SaveInode(IrpContext, Vcb, Inode)) {
                    Status = STATUS_SUCCESS;
                }
            }

            ClearFlag(NotifyFilter, FILE_NOTIFY_CHANGE_LAST_ACCESS);
            Status = STATUS_SUCCESS;
        }

        break;

        case FileAllocationInformation:
        {
            PFILE_ALLOCATION_INFORMATION FAI = (PFILE_ALLOCATION_INFORMATION)Buffer;
            LARGE_INTEGER  AllocationSize;

            if (IsMcbDirectory(Mcb) || IsMcbSpecialFile(Mcb)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            } else {
                Status = STATUS_SUCCESS;
            }

            /* set Mcb to it's target */
            if (IsMcbSymLink(Mcb)) {
                ASSERT(Fcb->Mcb == Mcb->Target);
            }
            Mcb = Fcb->Mcb;

            /* get user specified allocationsize aligned with BLOCK_SIZE */
            AllocationSize.QuadPart = CEILING_ALIGNED(ULONGLONG,
                                      (ULONGLONG)FAI->AllocationSize.QuadPart,
                                      (ULONGLONG)BLOCK_SIZE);

            if (AllocationSize.QuadPart > Fcb->Header.AllocationSize.QuadPart) {

                Status = Ext2ExpandFile(IrpContext, Vcb, Mcb, &AllocationSize); 
                Fcb->Header.AllocationSize = AllocationSize;
                NotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
                SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_SETINFO);

            } else if (AllocationSize.QuadPart < Fcb->Header.AllocationSize.QuadPart) {

                if (MmCanFileBeTruncated(&(Fcb->SectionObject), &AllocationSize))  {

                    /* truncate file blocks */
                    Status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &AllocationSize);

                    if (NT_SUCCESS(Status)) {
                        ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                    }

                    NotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
                    Fcb->Header.AllocationSize.QuadPart = AllocationSize.QuadPart;
                    if (Mcb->Inode.i_size > (loff_t)AllocationSize.QuadPart) {
                        Mcb->Inode.i_size = AllocationSize.QuadPart;
                    }
                    Fcb->Header.FileSize.QuadPart = Mcb->Inode.i_size;
                    if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart) {
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                    }

                } else {

                    Status = STATUS_USER_MAPPED_FILE;
                    DbgBreak();
                    _SEH2_LEAVE;
                }
            }

            if (NotifyFilter) {

                SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);
                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                }
            }

            DEBUG(DL_IO, ("Ext2SetInformation: %wZ NewSize=%I64xh AllocationSize=%I64xh "
                          "FileSize=%I64xh VDL=%I64xh i_size=%I64xh status = %xh\n",
                          &Fcb->Mcb->ShortName, AllocationSize.QuadPart,
                          Fcb->Header.AllocationSize.QuadPart,
                          Fcb->Header.FileSize.QuadPart, Fcb->Header.ValidDataLength.QuadPart,
                          Mcb->Inode.i_size, Status));
        }

        break;

        case FileEndOfFileInformation:
        {
            PFILE_END_OF_FILE_INFORMATION FEOFI = (PFILE_END_OF_FILE_INFORMATION) Buffer;
            LARGE_INTEGER NewSize, OldSize, EndOfFile;

            if (IsMcbDirectory(Mcb) || IsMcbSpecialFile(Mcb)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            } else {
                Status = STATUS_SUCCESS;
            }

            /* set Mcb to it's target */
            if (IsMcbSymLink(Mcb)) {
                ASSERT(Fcb->Mcb == Mcb->Target);
            }
            Mcb = Fcb->Mcb;

            OldSize = Fcb->Header.AllocationSize;
            EndOfFile = FEOFI->EndOfFile;

            if (IoStackLocation->Parameters.SetFile.AdvanceOnly) {

                if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
                    _SEH2_LEAVE;
                }

                if (EndOfFile.QuadPart > Fcb->Header.FileSize.QuadPart) {
                    EndOfFile.QuadPart = Fcb->Header.FileSize.QuadPart;
                }

                if (EndOfFile.QuadPart > Fcb->Header.ValidDataLength.QuadPart) {
                    Fcb->Header.ValidDataLength.QuadPart = EndOfFile.QuadPart;
                    NotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
                }

                _SEH2_LEAVE;
            }

            NewSize.QuadPart = CEILING_ALIGNED(ULONGLONG,
                                               EndOfFile.QuadPart, BLOCK_SIZE);

            if (NewSize.QuadPart > OldSize.QuadPart) {

                Fcb->Header.AllocationSize = NewSize;
                Status = Ext2ExpandFile(
                                 IrpContext,
                                 Vcb,
                                 Mcb,
                                 &(Fcb->Header.AllocationSize)
                             );
                NotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
                SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_SETINFO);


            } else if (NewSize.QuadPart == OldSize.QuadPart) {

                /* we are luck ;) */
                Status = STATUS_SUCCESS;

            } else {

                /* don't truncate file data since it's still being written */
                if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_WRITE)) {

                    Status = STATUS_SUCCESS;

                } else {

                    if (!MmCanFileBeTruncated(&(Fcb->SectionObject), &NewSize)) {
                        Status = STATUS_USER_MAPPED_FILE;
                        DbgBreak();
                        _SEH2_LEAVE;
                    }

                    /* truncate file blocks */
                    Status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &NewSize);

                    /* restore original file size */
                    if (NT_SUCCESS(Status)) {
                        ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                    }

                    /* update file allocateion size */
                    Fcb->Header.AllocationSize.QuadPart = NewSize.QuadPart;

                    ASSERT((loff_t)NewSize.QuadPart >= Mcb->Inode.i_size);
                    if ((loff_t)Fcb->Header.FileSize.QuadPart < Mcb->Inode.i_size) {
                        Fcb->Header.FileSize.QuadPart = Mcb->Inode.i_size;
                    }
                    if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart) {
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                    }

                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                    SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                }

                NotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
            }

            if (NT_SUCCESS(Status)) {

                Fcb->Header.FileSize.QuadPart = Mcb->Inode.i_size = EndOfFile.QuadPart;
                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                }

                if (Fcb->Header.FileSize.QuadPart >= 0x80000000 &&
                        !IsFlagOn(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
                    SetFlag(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE);
                    Ext2SaveSuper(IrpContext, Vcb);
                }

                SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                NotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
            }


            Ext2SaveInode( IrpContext, Vcb, &Mcb->Inode);

            DEBUG(DL_IO, ("Ext2SetInformation: FileEndOfFileInformation %wZ EndofFile=%I64xh "
                          "AllocatieonSize=%I64xh FileSize=%I64xh VDL=%I64xh i_size=%I64xh status = %xh\n",
                          &Fcb->Mcb->ShortName, EndOfFile.QuadPart, Fcb->Header.AllocationSize.QuadPart,
                          Fcb->Header.FileSize.QuadPart, Fcb->Header.ValidDataLength.QuadPart,
                          Mcb->Inode.i_size, Status));
        }

        break;

        case FileValidDataLengthInformation:
        {
            PFILE_VALID_DATA_LENGTH_INFORMATION FVDL = (PFILE_VALID_DATA_LENGTH_INFORMATION) Buffer;
            LARGE_INTEGER NewVDL;

            if (IsMcbDirectory(Mcb) || IsMcbSpecialFile(Mcb)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            } else {
                Status = STATUS_SUCCESS;
            }

            NewVDL = FVDL->ValidDataLength;
            if ((NewVDL.QuadPart < Fcb->Header.ValidDataLength.QuadPart)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
            if (NewVDL.QuadPart > Fcb->Header.FileSize.QuadPart)
                NewVDL = Fcb->Header.FileSize;

            if (!MmCanFileBeTruncated(FileObject->SectionObjectPointer,
                                      &NewVDL)) {
                Status = STATUS_USER_MAPPED_FILE;
                _SEH2_LEAVE;
            }

            Fcb->Header.ValidDataLength = NewVDL;
            FileObject->Flags |= FO_FILE_MODIFIED;
            if (CcIsFileCached(FileObject)) {
                CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
            }
        }

        break;

        case FileDispositionInformation:
        {
            PFILE_DISPOSITION_INFORMATION FDI = (PFILE_DISPOSITION_INFORMATION)Buffer;

            Status = Ext2SetDispositionInfo(IrpContext, Vcb, Fcb, Ccb, FDI->DeleteFile);

            DEBUG(DL_INF, ( "Ext2SetInformation: SetDispositionInformation: DeleteFile=%d %wZ status = %xh\n",
                            FDI->DeleteFile, &Mcb->ShortName, Status));
        }

        break;

        case FileRenameInformation:
        {
            Status = Ext2SetRenameInfo(IrpContext, Vcb, Fcb, Ccb);
        }

        break;

        //
        // This is the only set file information request supported on read
        // only file systems
        //
        case FilePositionInformation:
        {
            PFILE_POSITION_INFORMATION FilePositionInformation;

            if (Length < sizeof(FILE_POSITION_INFORMATION)) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            FilePositionInformation = (PFILE_POSITION_INFORMATION) Buffer;

            if ((FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING)) &&
                    (FilePositionInformation->CurrentByteOffset.LowPart &
                     DeviceObject->AlignmentRequirement) ) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            FileObject->CurrentByteOffset =
                FilePositionInformation->CurrentByteOffset;

            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        break;

        case FileLinkInformation:

            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            DEBUG(DL_WRN, ( "Ext2SetInformation: invalid class: %d\n",
                            FileInformationClass));
            Status = STATUS_INVALID_PARAMETER;/* STATUS_INVALID_INFO_CLASS; */
        }

    } _SEH2_FINALLY {

        if (FcbPagingIoResourceAcquired) {
            ExReleaseResourceLite(&Fcb->PagingIoResource);
        }

        if (NT_SUCCESS(Status) && (NotifyFilter != 0)) {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                NotifyFilter,
                FILE_ACTION_MODIFIED );

        }

        if (FcbMainResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (VcbMainResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING ||
                    Status == STATUS_CANT_WAIT ) {
                DbgBreak();
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext,  Status);
            }
        }
    } _SEH2_END;

    return Status;
}

ULONG
Ext2TotalBlocks(
    PEXT2_VCB         Vcb,
    PLARGE_INTEGER    Size,
    PULONG            pMeta
)
{
    ULONG Blocks, Meta =0, Remain;

    Blocks = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);
    if (Blocks <= EXT2_NDIR_BLOCKS)
        goto errorout;
    Blocks -= EXT2_NDIR_BLOCKS;

    Meta += 1;
    if (Blocks <= Vcb->max_blocks_per_layer[1]) {
        goto errorout;
    }
    Blocks -= Vcb->max_blocks_per_layer[1];

level2:

    if (Blocks <= Vcb->max_blocks_per_layer[2]) {
        Meta += 1 + ((Blocks + BLOCK_SIZE/4 - 1) >> (BLOCK_BITS - 2));
        goto errorout;
    }
    Meta += 1 + BLOCK_SIZE/4;
    Blocks -= Vcb->max_blocks_per_layer[2];

    if (Blocks > Vcb->max_blocks_per_layer[3]) {
        Blocks  = Vcb->max_blocks_per_layer[3];
    }

    ASSERT(Vcb->max_blocks_per_layer[2]);
    Remain = Blocks % Vcb->max_blocks_per_layer[2];
    Blocks = Blocks / Vcb->max_blocks_per_layer[2];
    Meta += 1 + Blocks * (1 + BLOCK_SIZE/4);
    if (Remain) {
        Blocks = Remain;
        goto level2;
    }

errorout:

    if (pMeta)
        *pMeta = Meta;
    Blocks = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);
    return (Blocks + Meta);
}

NTSTATUS
Ext2BlockMap(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock,
    OUT PULONG              Number
)
{
	NTSTATUS status;

	if (INODE_HAS_EXTENT(&Mcb->Inode)) {
        status = Ext2MapExtent(IrpContext, Vcb, Mcb, Index,
                               bAlloc, pBlock, Number );
	} else {
        status = Ext2MapIndirect(IrpContext, Vcb, Mcb, Index,
                                 bAlloc, pBlock, Number );
    }

	return status;
}


NTSTATUS
Ext2ExpandFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    Start = 0;
    ULONG    End = 0;

    Start = (ULONG)((Mcb->Inode.i_size + BLOCK_SIZE - 1) >> BLOCK_BITS);
    End = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);

    /* it's a truncate operation, not expanding */
    if (Start >= End) {
        Size->QuadPart = ((LONGLONG) Start) << BLOCK_BITS;
        return STATUS_SUCCESS;
    }

	/* ignore special files */
	if (IsMcbSpecialFile(Mcb)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

	/* expandind file extents */ 
    if (INODE_HAS_EXTENT(&Mcb->Inode)) {

        status = Ext2ExpandExtent(IrpContext, Vcb, Mcb, Start, End, Size);

    } else {

        BOOLEAN do_expand;

#if EXT2_PRE_ALLOCATION_SUPPORT
        do_expand = TRUE;
#else
        do_expand = (IrpContext->MajorFunction == IRP_MJ_WRITE) ||
                    IsMcbDirectory(Mcb);
#endif
        if (!do_expand)
            goto errorout;

        status = Ext2ExpandIndirect(IrpContext, Vcb, Mcb, Start, End, Size);
    }

errorout:
    return status;
}


NTSTATUS
Ext2TruncateFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (INODE_HAS_EXTENT(&Mcb->Inode)) {
		status = Ext2TruncateExtent(IrpContext, Vcb, Mcb, Size);
    } else {
		status = Ext2TruncateIndirect(IrpContext, Vcb, Mcb, Size);
	}

    /* check and clear data/meta mcb extents */
    if (Size->QuadPart == 0) {

        /* check and remove all data extents */
        if (Ext2ListExtents(&Mcb->Extents)) {
            DbgBreak();
        }
        Ext2ClearAllExtents(&Mcb->Extents);
        /* check and remove all meta extents */
        if (Ext2ListExtents(&Mcb->MetaExts)) {
            DbgBreak();
        }
        Ext2ClearAllExtents(&Mcb->MetaExts);
        ClearLongFlag(Mcb->Flags, MCB_ZONE_INITED);
    }

    return status;
}

NTSTATUS
Ext2IsFileRemovable(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Fcb,
    IN PEXT2_CCB            Ccb
)
{
    PEXT2_MCB Mcb = Fcb->Mcb;

    if (Mcb->Inode.i_ino == EXT2_ROOT_INO) {
        return STATUS_CANNOT_DELETE;
    }

    if (IsMcbDirectory(Mcb)) {
        if (!Ext2IsDirectoryEmpty(IrpContext, Vcb, Mcb)) {
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    }

    if (!MmFlushImageSection(&Fcb->SectionObject,
                             MmFlushForDelete )) {
        return STATUS_CANNOT_DELETE;
    }

    if (IsMcbDirectory(Mcb)) {
        FsRtlNotifyFullChangeDirectory(
            Vcb->NotifySync,
            &Vcb->NotifyList,
            Ccb,
            NULL,
            FALSE,
            FALSE,
            0,
            NULL,
            NULL,
            NULL
        );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2SetDispositionInfo(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    PEXT2_CCB Ccb,
    BOOLEAN bDelete
)
{
    PIRP    Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS status = STATUS_SUCCESS;
    PEXT2_MCB  Mcb = Fcb->Mcb;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    DEBUG(DL_INF, ( "Ext2SetDispositionInfo: bDelete=%x\n", bDelete));

    if (bDelete) {

        DEBUG(DL_INF, ( "Ext2SetDispositionInformation: Removing %wZ.\n",
                        &Mcb->FullName));

        /* always allow deleting on symlinks */
        if (Ccb->SymLink == NULL) {
            status = Ext2IsFileRemovable(IrpContext, Vcb, Fcb, Ccb);
        }

        if (NT_SUCCESS(status)) {
            SetLongFlag(Fcb->Flags, FCB_DELETE_PENDING);
            IrpSp->FileObject->DeletePending = TRUE;
        }

    } else {

        ClearLongFlag(Fcb->Flags, FCB_DELETE_PENDING);
        IrpSp->FileObject->DeletePending = FALSE;
    }

    return status;
}

NTSTATUS
Ext2SetRenameInfo(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_FCB         Fcb,
    PEXT2_CCB         Ccb
)
{
    PEXT2_MCB               Mcb = Fcb->Mcb;

    PEXT2_FCB               TargetDcb = NULL;   /* Dcb of target directory */
    PEXT2_MCB               TargetMcb = NULL;
    PEXT2_FCB               ParentDcb = NULL;   /* Dcb of it's current parent */
    PEXT2_MCB               ParentMcb = NULL;

    PEXT2_FCB               ExistingFcb = NULL; /* Target file Fcb if it exists*/
    PEXT2_MCB               ExistingMcb = NULL;

    UNICODE_STRING          FileName;

    NTSTATUS                Status;

    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;

    PFILE_OBJECT            FileObject;
    PFILE_OBJECT            TargetObject;

    struct dentry          *NewEntry = NULL;

    BOOLEAN                 ReplaceIfExists;
    BOOLEAN                 bMove = FALSE;
    BOOLEAN                 bTargetRemoved = FALSE;

    BOOLEAN                 bNewTargetDcb = FALSE;
    BOOLEAN                 bNewParentDcb = FALSE;

    PFILE_RENAME_INFORMATION    FRI;

    if (Ccb->SymLink) {
        Mcb = Ccb->SymLink;
    }

    if (Mcb->Inode.i_ino == EXT2_ROOT_INO) {
        Status = STATUS_INVALID_PARAMETER;
        goto errorout;
    }

    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FileObject = IrpSp->FileObject;
    TargetObject = IrpSp->Parameters.SetFile.FileObject;
    ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;

    FRI = (PFILE_RENAME_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    if (TargetObject == NULL) {

        UNICODE_STRING  NewName;

        NewName.Buffer = FRI->FileName;
        NewName.MaximumLength = NewName.Length = (USHORT)FRI->FileNameLength;

        while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] == L'\\') {
            NewName.Buffer[NewName.Length/2 - 1] = 0;
            NewName.Length -= 2;
        }

        while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] != L'\\') {
            NewName.Length -= 2;
        }

        NewName.Buffer = (USHORT *)((UCHAR *)NewName.Buffer + NewName.Length);
        NewName.Length = (USHORT)(FRI->FileNameLength - NewName.Length);

        FileName = NewName;

        TargetMcb = Mcb->Parent;
        if (IsMcbSymLink(TargetMcb)) {
            TargetMcb = TargetMcb->Target;
            ASSERT(!IsMcbSymLink(TargetMcb));
        }

        if (TargetMcb == NULL || FileName.Length >= EXT2_NAME_LEN*2) {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto errorout;
        }

    } else {

        TargetDcb = (PEXT2_FCB)(TargetObject->FsContext);

        if (!TargetDcb || TargetDcb->Vcb != Vcb) {

            DbgBreak();

            Status = STATUS_INVALID_PARAMETER;
            goto errorout;
        }

        TargetMcb = TargetDcb->Mcb;
        FileName = TargetObject->FileName;
    }

    if (FsRtlDoesNameContainWildCards(&FileName)) {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto errorout;
    }

    if (TargetMcb->Inode.i_ino == Mcb->Parent->Inode.i_ino) {
        if (FsRtlAreNamesEqual( &FileName,
                                &(Mcb->ShortName),
                                FALSE,
                                NULL )) {
            Status = STATUS_SUCCESS;
            goto errorout;
        }
    } else {
        bMove = TRUE;
    }

    TargetDcb = TargetMcb->Fcb;
    if (TargetDcb == NULL) {
        TargetDcb = Ext2AllocateFcb(Vcb, TargetMcb);
        if (TargetDcb) {
            Ext2ReferXcb(&TargetDcb->ReferenceCount);
            bNewTargetDcb = TRUE;
        }
    }
    if (TargetDcb) {
        SetLongFlag(TargetDcb->Flags, FCB_STATE_BUSY);
    }

    ParentMcb = Mcb->Parent;
    ParentDcb = ParentMcb->Fcb;

    if ((TargetMcb->Inode.i_ino != ParentMcb->Inode.i_ino)) {

        if (ParentDcb == NULL) {
            ParentDcb = Ext2AllocateFcb(Vcb, ParentMcb);
            if (ParentDcb) {
                Ext2ReferXcb(&ParentDcb->ReferenceCount);
                bNewParentDcb = TRUE;
            }
        }
        if (ParentDcb) {
            SetLongFlag(ParentDcb->Flags, FCB_STATE_BUSY);
        }
    }

    if (!TargetDcb || !ParentDcb) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    DEBUG(DL_RES, ("Ext2SetRenameInfo: rename %wZ to %wZ\\%wZ\n",
                   &Mcb->FullName, &TargetMcb->FullName, &FileName));

    Status = Ext2LookupFile(
                 IrpContext,
                 Vcb,
                 &FileName,
                 TargetMcb,
                 &ExistingMcb,
                 0
             );

    if (NT_SUCCESS(Status) && ExistingMcb != Mcb) {

        if (!ReplaceIfExists) {

            Status = STATUS_OBJECT_NAME_COLLISION;
            DEBUG(DL_RES, ("Ext2SetRenameInfo: Target file %wZ exists\n",
                           &ExistingMcb->FullName));
            goto errorout;

        } else {

            if ( (ExistingFcb = ExistingMcb->Fcb) && !IsMcbSymLink(ExistingMcb) ) {

                Status = Ext2IsFileRemovable(IrpContext, Vcb, ExistingFcb, Ccb);
                if (!NT_SUCCESS(Status)) {
                    DEBUG(DL_REN, ("Ext2SetRenameInfo: Target file %wZ cannot be removed.\n",
                                   &ExistingMcb->FullName));
                    goto errorout;
                }
            }

            Status = Ext2DeleteFile(IrpContext, Vcb, ExistingFcb, ExistingMcb);
            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to delete %wZ with status: %xh.\n",
                               &FileName, Status));

                goto errorout;
            }

            bTargetRemoved = TRUE;
        }
    }

    /* remove directory entry of old name */
    Status = Ext2RemoveEntry(IrpContext, Vcb, ParentDcb, Mcb);
    if (!NT_SUCCESS(Status)) {
        DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to remove entry %wZ with status %xh.\n",
                       &Mcb->FullName, Status));
        DbgBreak();
        goto errorout;
    }

    /* add new entry for new target name */
    Status = Ext2AddEntry(IrpContext, Vcb, TargetDcb, &Mcb->Inode, &FileName, &NewEntry);
    if (!NT_SUCCESS(Status)) {
        DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to add entry for %wZ with status: %xh.\n",
                       &FileName, Status));
        Ext2AddEntry(IrpContext, Vcb, ParentDcb, &Mcb->Inode, &Mcb->ShortName, &NewEntry);
        goto errorout;
    }

    /* correct the inode number in ..  entry */
    if (IsMcbDirectory(Mcb)) {
        Status = Ext2SetParentEntry(
                     IrpContext, Vcb, Fcb,
                     ParentMcb->Inode.i_ino,
                     TargetMcb->Inode.i_ino );
        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to set parent refer of %wZ with %xh.\n",
                           &Mcb->FullName, Status));
            DbgBreak();
            goto errorout;
        }
    }

    /* Update current dentry from the newly created one. We need keep the original
       dentry to assure children's links are valid if current entry is a directory */
    if (Mcb->de) {
        char *np = Mcb->de->d_name.name;
        *(Mcb->de) = *NewEntry;
        NewEntry->d_name.name = np;
    }

    if (bTargetRemoved) {
        Ext2NotifyReportChange(
            IrpContext,
            Vcb,
            ExistingMcb,
            (IsMcbDirectory(ExistingMcb) ?
             FILE_NOTIFY_CHANGE_DIR_NAME :
             FILE_NOTIFY_CHANGE_FILE_NAME ),
            FILE_ACTION_REMOVED);
    }

    if (NT_SUCCESS(Status)) {

        if (bMove) {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                (IsDirectory(Fcb) ?
                 FILE_NOTIFY_CHANGE_DIR_NAME :
                 FILE_NOTIFY_CHANGE_FILE_NAME ),
                FILE_ACTION_REMOVED);

        } else {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                (IsDirectory(Fcb) ?
                 FILE_NOTIFY_CHANGE_DIR_NAME :
                 FILE_NOTIFY_CHANGE_FILE_NAME ),
                FILE_ACTION_RENAMED_OLD_NAME);

        }

        if (TargetMcb->Inode.i_ino != ParentMcb->Inode.i_ino) {
            Ext2RemoveMcb(Vcb, Mcb);
            Ext2InsertMcb(Vcb, TargetMcb, Mcb);
        }

        if (!Ext2BuildName( &Mcb->ShortName,
                            &FileName, NULL     )) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        if (!Ext2BuildName( &Mcb->FullName,
                            &FileName,
                            &TargetMcb->FullName)) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        if (bMove) {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                (IsDirectory(Fcb) ?
                 FILE_NOTIFY_CHANGE_DIR_NAME :
                 FILE_NOTIFY_CHANGE_FILE_NAME ),
                FILE_ACTION_ADDED);
        } else {
            Ext2NotifyReportChange(
                IrpContext,
                Vcb,
                Mcb,
                (IsDirectory(Fcb) ?
                 FILE_NOTIFY_CHANGE_DIR_NAME :
                 FILE_NOTIFY_CHANGE_FILE_NAME ),
                FILE_ACTION_RENAMED_NEW_NAME  );

        }
    }

errorout:

    if (NewEntry)
        Ext2FreeEntry(NewEntry);

    if (TargetDcb) {
        if (ParentDcb && ParentDcb->Inode->i_ino != TargetDcb->Inode->i_ino) {
            ClearLongFlag(ParentDcb->Flags, FCB_STATE_BUSY);
        }
        ClearLongFlag(TargetDcb->Flags, FCB_STATE_BUSY);
    }

    if (bNewTargetDcb) {
        ASSERT(TargetDcb != NULL);
        if (Ext2DerefXcb(&TargetDcb->ReferenceCount) == 0) {
            Ext2FreeFcb(TargetDcb);
            TargetDcb = NULL;
        } else {
            DEBUG(DL_RES, ( "Ext2SetRenameInfo: TargetDcb is resued by other threads.\n"));
        }
    }

    if (bNewParentDcb) {
        ASSERT(ParentDcb != NULL);
        if (Ext2DerefXcb(&ParentDcb->ReferenceCount) == 0) {
            Ext2FreeFcb(ParentDcb);
            ParentDcb = NULL;
        } else {
            DEBUG(DL_RES, ( "Ext2SetRenameInfo: ParentDcb is resued by other threads.\n"));
        }
    }

    if (ExistingMcb)
        Ext2DerefMcb(ExistingMcb);

    return Status;
}

ULONG
Ext2InodeType(PEXT2_MCB Mcb)
{
    if (IsMcbSymLink(Mcb)) {
        return EXT2_FT_SYMLINK;
    }

    if (IsMcbDirectory(Mcb)) {
        return EXT2_FT_DIR;
    }

    return EXT2_FT_REG_FILE;
}

NTSTATUS
Ext2DeleteFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_FCB         Fcb,
    PEXT2_MCB         Mcb
)
{
    PEXT2_FCB       Dcb = NULL;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    BOOLEAN         VcbResourceAcquired = FALSE;
    BOOLEAN         FcbPagingIoAcquired = FALSE;
    BOOLEAN         FcbResourceAcquired = FALSE;
    BOOLEAN         DcbResourceAcquired = FALSE;

    LARGE_INTEGER   Size;
    LARGE_INTEGER   SysTime;

    BOOLEAN         bNewDcb = FALSE;

    DEBUG(DL_INF, ( "Ext2DeleteFile: File %wZ (%xh) will be deleted!\n",
                    &Mcb->FullName, Mcb->Inode.i_ino));

    if (IsFlagOn(Mcb->Flags, MCB_FILE_DELETED)) {
        return STATUS_SUCCESS;
    }

    if (!IsMcbSymLink(Mcb) && IsMcbDirectory(Mcb)) {
        if (!Ext2IsDirectoryEmpty(IrpContext, Vcb, Mcb)) {
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    }


    _SEH2_TRY {

        Ext2ReferMcb(Mcb);

        ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
        VcbResourceAcquired = TRUE;

        if (!(Dcb = Mcb->Parent->Fcb)) {
            Dcb = Ext2AllocateFcb(Vcb, Mcb->Parent);
            if (Dcb) {
                Ext2ReferXcb(&Dcb->ReferenceCount);
                bNewDcb = TRUE;
            }
        }

        if (Dcb) {
            SetLongFlag(Dcb->Flags, FCB_STATE_BUSY);
            DcbResourceAcquired =
                ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

            /* remove it's entry form it's parent */
            Status = Ext2RemoveEntry(IrpContext, Vcb, Dcb, Mcb);
        }

        if (NT_SUCCESS(Status)) {

            SetFlag(Mcb->Flags, MCB_FILE_DELETED);
            Ext2RemoveMcb(Vcb, Mcb);

            if (Fcb) {
                FcbResourceAcquired =
                    ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);

                FcbPagingIoAcquired =
                    ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
            }

            if (DcbResourceAcquired) {
                ExReleaseResourceLite(&Dcb->MainResource);
                DcbResourceAcquired = FALSE;
            }

            if (VcbResourceAcquired) {
                ExReleaseResourceLite(&Vcb->MainResource);
                VcbResourceAcquired = FALSE;
            }

            if (IsMcbSymLink(Mcb)) {
                if (Mcb->Inode.i_nlink > 0) {
                    Status = STATUS_CANNOT_DELETE;
                    _SEH2_LEAVE;
                }
            } else if (!IsMcbDirectory(Mcb)) {
                if (Mcb->Inode.i_nlink > 0) {
                    _SEH2_LEAVE;
                }
            } else {
                if (Mcb->Inode.i_nlink >= 2) {
                    _SEH2_LEAVE;
                }
            }

            if (S_ISLNK(Mcb->Inode.i_mode)) {

                /* for symlink, we should do differenctly  */
                if (Mcb->Inode.i_size > EXT2_LINKLEN_IN_INODE) {
                    Size.QuadPart = (LONGLONG)0;
                    Status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &Size);
                }

            } else {

                /* truncate file size */
                Size.QuadPart = (LONGLONG)0;
                Status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &Size);

                /* check file offset mappings */
                DEBUG(DL_EXT, ("Ext2DeleteFile ...: %wZ\n", &Mcb->FullName));

                if (Fcb) {
                    Fcb->Header.AllocationSize.QuadPart = Size.QuadPart;
                    if (Fcb->Header.FileSize.QuadPart > Size.QuadPart) {
                        Fcb->Header.FileSize.QuadPart = Size.QuadPart;
                        Fcb->Mcb->Inode.i_size = Size.QuadPart;
                    }
                    if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart) {
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                    }
                } else if (Mcb) {
                    /* Update the inode's data length . It should be ZERO if succeeds. */
                    if (Mcb->Inode.i_size > (loff_t)Size.QuadPart) {
                        Mcb->Inode.i_size = Size.QuadPart;
                    }
                }
            }

            /* set delete time and free the inode */
            KeQuerySystemTime(&SysTime);
            Mcb->Inode.i_nlink = 0;
            Mcb->Inode.i_dtime = Ext2LinuxTime(SysTime);
            Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);
            Ext2FreeInode(IrpContext, Vcb, Mcb->Inode.i_ino, Ext2InodeType(Mcb));
        }

    } _SEH2_FINALLY {

        if (FcbPagingIoAcquired) {
            ExReleaseResourceLite(&Fcb->PagingIoResource);
        }

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (DcbResourceAcquired) {
            ExReleaseResourceLite(&Dcb->MainResource);
        }

        if (Dcb) {
            ClearLongFlag(Dcb->Flags, FCB_STATE_BUSY);
            if (bNewDcb) {
                if (Ext2DerefXcb(&Dcb->ReferenceCount) == 0) {
                    Ext2FreeFcb(Dcb);
                } else {
                    DEBUG(DL_ERR, ( "Ext2DeleteFile: Dcb %wZ used by other threads.\n",
                                    &Mcb->FullName ));
                }
            }
        }
        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        Ext2DerefMcb(Mcb);
    } _SEH2_END;

    DEBUG(DL_INF, ( "Ext2DeleteFile: %wZ Succeed... EXT2SB->S_FREE_BLOCKS = %I64xh .\n",
                    &Mcb->FullName, ext3_free_blocks_count(SUPER_BLOCK)));

    return Status;
}

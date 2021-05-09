/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             fileinfo.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdQueryInformation)
#pragma alloc_text(PAGE, RfsdSetInformation)
#if !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdExpandFile)
#pragma alloc_text(PAGE, RfsdTruncateFile)
#pragma alloc_text(PAGE, RfsdSetDispositionInfo)
#pragma alloc_text(PAGE, RfsdSetRenameInfo)
#pragma alloc_text(PAGE, RfsdDeleteFile)
#endif // !RFSD_READ_ONLY
#endif

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdQueryInformation (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT            FileObject;
    PRFSD_VCB               Vcb;
    PRFSD_FCB               Fcb = 0;
    PRFSD_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 FcbResourceAcquired = FALSE;
    LONGLONG                FileSize;
    LONGLONG                AllocationSize;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == RFSDVCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

        Vcb = Fcb->Vcb;

/*        
        if ( !IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) &&
             !FlagOn(Fcb->Flags, FCB_PAGE_FILE))
*/
        {
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous
                )) {

                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            FcbResourceAcquired = TRUE;
        }
        
        Ccb = (PRFSD_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb != NULL);
        
        ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
            (Ccb->Identifier.Size == sizeof(RFSD_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        FileInformationClass =
            IoStackLocation->Parameters.QueryFile.FileInformationClass;
        
        Length = IoStackLocation->Parameters.QueryFile.Length;
        
        Buffer = Irp->AssociatedIrp.SystemBuffer;
        
        RtlZeroMemory(Buffer, Length);

        FileSize  = (LONGLONG) Fcb->Inode->i_size;

        AllocationSize = CEILING_ALIGNED(FileSize, (ULONGLONG)Vcb->BlockSize);
       
        switch (FileInformationClass) {

        case FileBasicInformation:
            {
                PFILE_BASIC_INFORMATION FileBasicInformation;
                
                if (Length < sizeof(FILE_BASIC_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FileBasicInformation = (PFILE_BASIC_INFORMATION) Buffer;

                RtlZeroMemory(FileBasicInformation, sizeof(FILE_BASIC_INFORMATION));
                
                FileBasicInformation->CreationTime = RfsdSysTime(Fcb->Inode->i_ctime);
                
                FileBasicInformation->LastAccessTime = RfsdSysTime(Fcb->Inode->i_atime);
                
                FileBasicInformation->LastWriteTime = RfsdSysTime(Fcb->Inode->i_mtime);
                
                FileBasicInformation->ChangeTime = RfsdSysTime(Fcb->Inode->i_mtime);
                
                FileBasicInformation->FileAttributes = Fcb->RfsdMcb->FileAttr;
                
                Irp->IoStatus.Information = sizeof(FILE_BASIC_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }

#if (_WIN32_WINNT >= 0x0500)

        case FileAttributeTagInformation:
            {
                PFILE_ATTRIBUTE_TAG_INFORMATION FATI;
                
                if (Length < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FATI = (PFILE_ATTRIBUTE_TAG_INFORMATION) Buffer;
                
                FATI->FileAttributes = Fcb->RfsdMcb->FileAttr;
                FATI->ReparseTag = 0;
                
                Irp->IoStatus.Information = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
#endif // (_WIN32_WINNT >= 0x0500)

        case FileStandardInformation:
            {
                PFILE_STANDARD_INFORMATION FileStandardInformation;
                
                if (Length < sizeof(FILE_STANDARD_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FileStandardInformation = (PFILE_STANDARD_INFORMATION) Buffer;
                
                FileStandardInformation->AllocationSize.QuadPart = AllocationSize;
                FileStandardInformation->EndOfFile.QuadPart = FileSize;
                
                FileStandardInformation->NumberOfLinks = Fcb->Inode->i_links_count;
                
                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                    FileStandardInformation->DeletePending = FALSE;
                else
                    FileStandardInformation->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);                
                
                if (Fcb->RfsdMcb->FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
                    FileStandardInformation->Directory = TRUE;
                } else {
                    FileStandardInformation->Directory = FALSE;
                }
                
                Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FileInternalInformation:
            {
                PFILE_INTERNAL_INFORMATION FileInternalInformation;
                
                if (Length < sizeof(FILE_INTERNAL_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FileInternalInformation = (PFILE_INTERNAL_INFORMATION) Buffer;
                
                // The "inode number"
				FileInternalInformation->IndexNumber.LowPart = Fcb->RfsdMcb->Key.k_dir_id;
				FileInternalInformation->IndexNumber.HighPart = Fcb->RfsdMcb->Key.k_objectid;
                
                Irp->IoStatus.Information = sizeof(FILE_INTERNAL_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FileEaInformation:
            {
                PFILE_EA_INFORMATION FileEaInformation;
                
                if (Length < sizeof(FILE_EA_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FileEaInformation = (PFILE_EA_INFORMATION) Buffer;
                
                // Romfs doesn't have any extended attributes
                FileEaInformation->EaSize = 0;
                
                Irp->IoStatus.Information = sizeof(FILE_EA_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FileNameInformation:
            {
                PFILE_NAME_INFORMATION FileNameInformation;
                
                if (Length < sizeof(FILE_NAME_INFORMATION) +
                    Fcb->RfsdMcb->ShortName.Length - sizeof(WCHAR)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FileNameInformation = (PFILE_NAME_INFORMATION) Buffer;
                
                FileNameInformation->FileNameLength = Fcb->RfsdMcb->ShortName.Length;
                
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->RfsdMcb->ShortName.Buffer,
                    Fcb->RfsdMcb->ShortName.Length );
                
                Irp->IoStatus.Information = sizeof(FILE_NAME_INFORMATION) +
                    Fcb->RfsdMcb->ShortName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FilePositionInformation:
            {
                PFILE_POSITION_INFORMATION FilePositionInformation;
                
                if (Length < sizeof(FILE_POSITION_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FilePositionInformation = (PFILE_POSITION_INFORMATION) Buffer;
                
                FilePositionInformation->CurrentByteOffset =
                    FileObject->CurrentByteOffset;
                
                Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FileAllInformation:
            {
                PFILE_ALL_INFORMATION       FileAllInformation;
                PFILE_BASIC_INFORMATION     FileBasicInformation;
                PFILE_STANDARD_INFORMATION  FileStandardInformation;
                PFILE_INTERNAL_INFORMATION  FileInternalInformation;
                PFILE_EA_INFORMATION        FileEaInformation;
                PFILE_POSITION_INFORMATION  FilePositionInformation;
                PFILE_NAME_INFORMATION      FileNameInformation;
                
                if (Length < sizeof(FILE_ALL_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FileAllInformation = (PFILE_ALL_INFORMATION) Buffer;
                
                FileBasicInformation =
                    &FileAllInformation->BasicInformation;
                
                FileStandardInformation =
                    &FileAllInformation->StandardInformation;
                
                FileInternalInformation =
                    &FileAllInformation->InternalInformation;
                
                FileEaInformation =
                    &FileAllInformation->EaInformation;
                
                FilePositionInformation =
                    &FileAllInformation->PositionInformation;
                
                FileNameInformation =
                    &FileAllInformation->NameInformation;
                
                FileBasicInformation->CreationTime = RfsdSysTime(Fcb->Inode->i_ctime);
                
                FileBasicInformation->LastAccessTime = RfsdSysTime(Fcb->Inode->i_atime);
                
                FileBasicInformation->LastWriteTime = RfsdSysTime(Fcb->Inode->i_mtime);
                
                FileBasicInformation->ChangeTime = RfsdSysTime(Fcb->Inode->i_mtime);
                
                FileBasicInformation->FileAttributes = Fcb->RfsdMcb->FileAttr;
                
                FileStandardInformation->AllocationSize.QuadPart = AllocationSize;
                
                FileStandardInformation->EndOfFile.QuadPart = FileSize;
                
                FileStandardInformation->NumberOfLinks = Fcb->Inode->i_links_count;

                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                    FileStandardInformation->DeletePending = FALSE;
                else
                    FileStandardInformation->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
                
                if (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
                    FileStandardInformation->Directory = TRUE;
                } else {
                    FileStandardInformation->Directory = FALSE;
                }
                
                // The "inode number"
				FileInternalInformation->IndexNumber.LowPart = Fcb->RfsdMcb->Key.k_dir_id;
				FileInternalInformation->IndexNumber.HighPart = Fcb->RfsdMcb->Key.k_objectid;
                
                // Romfs doesn't have any extended attributes
                FileEaInformation->EaSize = 0;
                
                FilePositionInformation->CurrentByteOffset =
                    FileObject->CurrentByteOffset;
                
                if (Length < sizeof(FILE_ALL_INFORMATION) +
                    Fcb->RfsdMcb->ShortName.Length - sizeof(WCHAR)) {
                    Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    _SEH2_LEAVE;
                }
                
                FileNameInformation->FileNameLength = Fcb->RfsdMcb->ShortName.Length;
                
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->RfsdMcb->ShortName.Buffer,
                    Fcb->RfsdMcb->ShortName.Length
                    );
                
                Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION) +
                    Fcb->RfsdMcb->ShortName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
        
        /*
        case FileAlternateNameInformation:
            {
        // TODO: [ext2fsd] Handle FileAlternateNameInformation
        
          // Here we would like to use RtlGenerate8dot3Name but I don't
          // know how to use the argument PGENERATE_NAME_CONTEXT
          }
          */
          
        case FileNetworkOpenInformation:
        {
            PFILE_NETWORK_OPEN_INFORMATION FileNetworkOpenInformation;
            
            if (Length < sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                _SEH2_LEAVE;
            }
            
            FileNetworkOpenInformation =
                (PFILE_NETWORK_OPEN_INFORMATION) Buffer;
            
            FileNetworkOpenInformation->CreationTime = RfsdSysTime(Fcb->Inode->i_ctime);
            
            FileNetworkOpenInformation->LastAccessTime = RfsdSysTime(Fcb->Inode->i_atime);
            
            FileNetworkOpenInformation->LastWriteTime = RfsdSysTime(Fcb->Inode->i_mtime);
            
            FileNetworkOpenInformation->ChangeTime = RfsdSysTime(Fcb->Inode->i_mtime);
            
            FileNetworkOpenInformation->AllocationSize.QuadPart = AllocationSize;
            
            FileNetworkOpenInformation->EndOfFile.QuadPart = FileSize;
            
            FileNetworkOpenInformation->FileAttributes = Fcb->RfsdMcb->FileAttr;
            
            Irp->IoStatus.Information =
                sizeof(FILE_NETWORK_OPEN_INFORMATION);
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }
        
        default:
        Status = STATUS_INVALID_INFO_CLASS;
        }

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                RfsdCompleteIrpContext(IrpContext,  Status);
            }
        }
    } _SEH2_END;
    
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdSetInformation (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PRFSD_VCB               Vcb = 0;
    PFILE_OBJECT            FileObject;
    PRFSD_FCB               Fcb = 0;
    PRFSD_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;

    ULONG                   NotifyFilter = 0;

    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 FcbMainResourceAcquired = FALSE;

    BOOLEAN                 VcbResourceAcquired = FALSE;
    BOOLEAN                 FcbPagingIoResourceAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == RFSDVCB) {
            DbgBreak();

            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        Ccb = (PRFSD_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb != NULL);
        
        ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
            (Ccb->Identifier.Size == sizeof(RFSD_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        FileInformationClass =
            IoStackLocation->Parameters.SetFile.FileInformationClass;
        
        Length = IoStackLocation->Parameters.SetFile.Length;
        
        Buffer = Irp->AssociatedIrp.SystemBuffer;

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {

            if (FileInformationClass == FileDispositionInformation ||
                FileInformationClass == FileRenameInformation ||
                FileInformationClass == FileLinkInformation)    {

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
                if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource,
                    IrpContext->IsSynchronous )) {

                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
            
                VcbResourceAcquired = TRUE;
            }

        } else if (!FlagOn(Fcb->Flags, FCB_PAGE_FILE)) {

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
            if (!ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            FcbMainResourceAcquired = TRUE;
        }
        
        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {

            if (FileInformationClass != FilePositionInformation) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                _SEH2_LEAVE;
            }
        }

        if (FileInformationClass == FileDispositionInformation ||
            FileInformationClass == FileRenameInformation ||
            FileInformationClass == FileLinkInformation ||
            FileInformationClass == FileAllocationInformation ||
            FileInformationClass == FileEndOfFileInformation) {

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
            if (!ExAcquireResourceExclusiveLite(
                &Fcb->PagingIoResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            FcbPagingIoResourceAcquired = TRUE;
        }
       
/*        
        if (FileInformationClass != FileDispositionInformation 
            && FlagOn(Fcb->Flags, FCB_DELETE_PENDING))
        {
            Status = STATUS_DELETE_PENDING;
            _SEH2_LEAVE;
        }
*/
        switch (FileInformationClass) {

#if !RFSD_READ_ONLY
#if 0
        case FileBasicInformation:
            {
                PFILE_BASIC_INFORMATION FBI = (PFILE_BASIC_INFORMATION) Buffer;               
                PRFSD_INODE RfsdInode = Fcb->Inode;

                if (FBI->CreationTime.QuadPart) {
                    RfsdInode->i_ctime = (ULONG)(RfsdInodeTime(FBI->CreationTime));
                }

                if (FBI->LastAccessTime.QuadPart) {
                    RfsdInode->i_atime = (ULONG)(RfsdInodeTime(FBI->LastAccessTime));
                }

                if (FBI->LastWriteTime.QuadPart) {
                    RfsdInode->i_mtime = (ULONG)(RfsdInodeTime(FBI->LastWriteTime));
                }

                if (IsFlagOn(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY)) {
                    RfsdSetReadOnly(Fcb->Inode->i_mode);
                    SetFlag(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_READONLY);
                } else {
                    RfsdSetWritable(Fcb->Inode->i_mode);
                    ClearFlag(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_READONLY);
                }

                if(RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, RfsdInode)) {
                    Status = STATUS_SUCCESS;
                }

                if (FBI->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                    SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                } else {
                    ClearFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                }

                NotifyFilter = FILE_NOTIFY_CHANGE_ATTRIBUTES |
                               FILE_NOTIFY_CHANGE_CREATION |
                               FILE_NOTIFY_CHANGE_LAST_ACCESS |
                               FILE_NOTIFY_CHANGE_LAST_WRITE ;

                Status = STATUS_SUCCESS;
            }

            break;

        case FileAllocationInformation:
            {
                PFILE_ALLOCATION_INFORMATION FAI = (PFILE_ALLOCATION_INFORMATION)Buffer;

                if (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    _SEH2_LEAVE;
                }

                if ( FAI->AllocationSize.QuadPart == 
                     Fcb->Header.AllocationSize.QuadPart) {

                    Status = STATUS_SUCCESS;

                } else if ( FAI->AllocationSize.QuadPart >
                          Fcb->Header.AllocationSize.QuadPart ) {

                    Status = RfsdExpandFile(
                                IrpContext,
                                Vcb, Fcb,
                                &(FAI->AllocationSize));

                    if (NT_SUCCESS(Status)) {

                        RfsdSaveInode( IrpContext,
                                       Vcb,
                                       Fcb->RfsdMcb->Inode,
                                       Fcb->Inode );
                    }

                } else {

                    if (MmCanFileBeTruncated(&(Fcb->SectionObject), &(FAI->AllocationSize)))  {

                        LARGE_INTEGER EndOfFile;

                        EndOfFile.QuadPart = FAI->AllocationSize.QuadPart +
                                             (LONGLONG)(Vcb->BlockSize - 1);

                        Status = RfsdTruncateFile(IrpContext, Vcb, Fcb, &(EndOfFile));

                        if (NT_SUCCESS(Status)) {

                            if ( FAI->AllocationSize.QuadPart < 
                                 Fcb->Header.FileSize.QuadPart) {
                                Fcb->Header.FileSize.QuadPart = 
                                                FAI->AllocationSize.QuadPart;
                            }

                            RfsdSaveInode( IrpContext,
                                           Vcb,
                                           Fcb->RfsdMcb->Inode,
                                           Fcb->Inode);
                        }

                    } else {

                        Status = STATUS_USER_MAPPED_FILE;
                        _SEH2_LEAVE;
                    }
                }

                if (NT_SUCCESS(Status)) {

                    CcSetFileSizes(FileObject, 
                            (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);

                    NotifyFilter = FILE_NOTIFY_CHANGE_SIZE |
                                   FILE_NOTIFY_CHANGE_LAST_WRITE ;

                }
                
            }

            break;

        case FileEndOfFileInformation:
            {
                PFILE_END_OF_FILE_INFORMATION FEOFI = (PFILE_END_OF_FILE_INFORMATION) Buffer;

                BOOLEAN CacheInitialized = FALSE;

                if (IsDirectory(Fcb)) {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    _SEH2_LEAVE;
                }

                if (FEOFI->EndOfFile.HighPart != 0) {
                    Status = STATUS_INVALID_PARAMETER;
                    _SEH2_LEAVE;
                }


                if (IoStackLocation->Parameters.SetFile.AdvanceOnly) {
                    Status = STATUS_SUCCESS;
                    _SEH2_LEAVE;
                }

                if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
                    (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
                    !FlagOn(Irp->Flags, IRP_PAGING_IO)) {

                    ASSERT( !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) );

                    CcInitializeCacheMap( 
                            FileObject,
                            (PCC_FILE_SIZES)&(Fcb->Header.AllocationSize),
                            FALSE,
                            &(RfsdGlobal->CacheManagerNoOpCallbacks),
                            Fcb );

                    CacheInitialized = TRUE;
                }

                if ( FEOFI->EndOfFile.QuadPart == 
                     Fcb->Header.AllocationSize.QuadPart) {

                    Status = STATUS_SUCCESS;

                } else if ( FEOFI->EndOfFile.QuadPart > 
                          Fcb->Header.AllocationSize.QuadPart) {

                    LARGE_INTEGER   FileSize = Fcb->Header.FileSize;

                    Status = RfsdExpandFile(IrpContext, Vcb, Fcb, &(FEOFI->EndOfFile));

                    if (NT_SUCCESS(Status)) {

                        Fcb->Header.FileSize.QuadPart = FEOFI->EndOfFile.QuadPart;

                        Fcb->Inode->i_size = FEOFI->EndOfFile.QuadPart;                        

                        Fcb->Header.ValidDataLength.QuadPart = 
                                        (LONGLONG)(0x7fffffffffffffff);

                        RfsdSaveInode( IrpContext,
                                       Vcb,
                                       Fcb->RfsdMcb->Inode,
                                       Fcb->Inode);


                        CcSetFileSizes(FileObject, 
                            (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));

                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);

                        RfsdZeroHoles( IrpContext, 
                                       Vcb, FileObject, 
                                       FileSize.QuadPart,
                                       Fcb->Header.AllocationSize.QuadPart - 
                                            FileSize.QuadPart );

                        NotifyFilter = FILE_NOTIFY_CHANGE_SIZE |
                                       FILE_NOTIFY_CHANGE_LAST_WRITE ;

                    }
                } else {

                    if (MmCanFileBeTruncated(&(Fcb->SectionObject), &(FEOFI->EndOfFile))) {

                        LARGE_INTEGER EndOfFile = FEOFI->EndOfFile;

                        EndOfFile.QuadPart = EndOfFile.QuadPart + 
                                             (LONGLONG)(Vcb->BlockSize - 1);

                        Status = RfsdTruncateFile(IrpContext, Vcb, Fcb, &(EndOfFile));

                        if (NT_SUCCESS(Status)) {

                            Fcb->Header.FileSize.QuadPart = FEOFI->EndOfFile.QuadPart;
                            Fcb->Inode->i_size = FEOFI->EndOfFile.QuadPart;                            

                            RfsdSaveInode( IrpContext,
                                           Vcb,
                                           Fcb->RfsdMcb->Inode,
                                           Fcb->Inode);

                            CcSetFileSizes(FileObject, 
                                    (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));

                            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);

                            NotifyFilter = FILE_NOTIFY_CHANGE_SIZE |
                                           FILE_NOTIFY_CHANGE_LAST_WRITE ;
                        }

                    } else {

                        Status = STATUS_USER_MAPPED_FILE;
                        _SEH2_LEAVE;
                    }
                }
            }

            break;

        case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION FDI = (PFILE_DISPOSITION_INFORMATION)Buffer;
                
                Status = RfsdSetDispositionInfo(IrpContext, Vcb, Fcb, FDI->DeleteFile);
            }

            break;

        case FileRenameInformation:
            {
                Status = RfsdSetRenameInfo(IrpContext, Vcb, Fcb);
            }

            break;
#endif // 0
#endif // !RFSD_READ_ONLY

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
            
        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }

    } _SEH2_FINALLY {
        
        if (FcbPagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread() );
        }
        
        if (NT_SUCCESS(Status) && (NotifyFilter != 0)) {
            RfsdNotifyReportChange(
                        IrpContext,
                        Vcb,
                        Fcb,
                        NotifyFilter,
                        FILE_ACTION_MODIFIED );

        }

        if (FcbMainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                RfsdCompleteIrpContext(IrpContext,  Status);
            }
        }
    } _SEH2_END;
    
    return Status;
}

#if !RFSD_READ_ONLY

NTSTATUS
RfsdExpandFile( PRFSD_IRP_CONTEXT IrpContext, 
                PRFSD_VCB Vcb, PRFSD_FCB Fcb,
                PLARGE_INTEGER AllocationSize)
{
    ULONG    dwRet = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (AllocationSize->QuadPart <= Fcb->Header.AllocationSize.QuadPart) {
        return Status;
    }

    if (((LONGLONG)SUPER_BLOCK->s_free_blocks_count) * Vcb->BlockSize <=
        (AllocationSize->QuadPart - Fcb->Header.AllocationSize.QuadPart)  ) {
        RfsdPrint((DBG_ERROR, "RfsdExpandFile: There is no enough disk space available.\n"));
        return STATUS_DISK_FULL;
    }

    while (NT_SUCCESS(Status) && (AllocationSize->QuadPart > Fcb->Header.AllocationSize.QuadPart)) {
        Status = RfsdExpandInode(IrpContext, Vcb, Fcb, &dwRet);
    }
    
    return Status;
}

NTSTATUS
RfsdTruncateFile( PRFSD_IRP_CONTEXT IrpContext,
                  PRFSD_VCB Vcb, PRFSD_FCB Fcb,
                  PLARGE_INTEGER AllocationSize)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    while (NT_SUCCESS(Status) && (AllocationSize->QuadPart <
                    Fcb->Header.AllocationSize.QuadPart)) {
        Status= RfsdTruncateInode(IrpContext, Vcb, Fcb);
    }

    return Status;
}

NTSTATUS
RfsdSetDispositionInfo(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            PRFSD_FCB Fcb,
            BOOLEAN bDelete)
{
    PIRP    Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    RfsdPrint((DBG_INFO, "RfsdSetDispositionInfo: bDelete=%x\n", bDelete));
    
    if (bDelete) {

        RfsdPrint((DBG_INFO, "RfsdSetDispositionInformation: MmFlushImageSection on %s.\n", 
                             Fcb->AnsiFileName.Buffer));

        if (!MmFlushImageSection( &Fcb->SectionObject,
                                  MmFlushForDelete )) {
            return STATUS_CANNOT_DELETE;
        }

        if (RFSD_IS_ROOT_KEY(Fcb->RfsdMcb->Key)) {
            return STATUS_CANNOT_DELETE;
        }

        if (IsDirectory(Fcb)) {
            if (!RfsdIsDirectoryEmpty(Vcb, Fcb)) {
                return STATUS_DIRECTORY_NOT_EMPTY;
            }
        }

        SetFlag(Fcb->Flags, FCB_DELETE_PENDING);
        IrpSp->FileObject->DeletePending = TRUE;

        if (IsDirectory(Fcb)) {
            FsRtlNotifyFullChangeDirectory( Vcb->NotifySync,
                                            &Vcb->NotifyList,
                                            Fcb,
                                            NULL,
                                            FALSE,
                                            FALSE,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL );
        }

    } else {

        ClearFlag(Fcb->Flags, FCB_DELETE_PENDING);
        IrpSp->FileObject->DeletePending = FALSE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RfsdSetRenameInfo(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB         Vcb,
            PRFSD_FCB         Fcb       )
{
    PRFSD_FCB               TargetDcb;
    PRFSD_MCB               TargetMcb;

    PRFSD_MCB               Mcb;
    RFSD_INODE              Inode;

    UNICODE_STRING          FileName;
    
    NTSTATUS                Status;

    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;

    PFILE_OBJECT            FileObject;
    PFILE_OBJECT            TargetObject;
    BOOLEAN                 ReplaceIfExists;

    BOOLEAN                 bMove = FALSE;

    PFILE_RENAME_INFORMATION    FRI;

    PAGED_CODE();
#if 0
    if (Fcb->RfsdMcb->Inode == RFSD_ROOT_INO) {
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

        TargetDcb = NULL;
        TargetMcb = Fcb->RfsdMcb->Parent;
        
        if (FileName.Length >= RFSD_NAME_LEN*sizeof(USHORT)) {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto errorout;
        }

    } else {

        TargetDcb = (PRFSD_FCB)(TargetObject->FsContext);

        if (!TargetDcb || TargetDcb->Vcb != Vcb) {

            DbgBreak();

            Status = STATUS_INVALID_PARAMETER;
            goto errorout;
        }

        TargetMcb = TargetDcb->RfsdMcb;

        FileName = TargetObject->FileName;
    }

    if (FsRtlDoesNameContainWildCards(&FileName)) {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto errorout;
    }

    if (TargetMcb->Inode == Fcb->RfsdMcb->Parent->Inode) {
        if (FsRtlAreNamesEqual( &FileName,
                                &(Fcb->RfsdMcb->ShortName),
                                FALSE,
                                NULL )) {
            Status = STATUS_SUCCESS;
            goto errorout;
        }
    } else {
        bMove = TRUE;
    }

    TargetDcb = TargetMcb->RfsdFcb;

    if (!TargetDcb)
        TargetDcb = RfsdCreateFcbFromMcb(Vcb, TargetMcb);

    if ((TargetMcb->Inode != Fcb->RfsdMcb->Parent->Inode) &&
        (Fcb->RfsdMcb->Parent->RfsdFcb == NULL)      ) {
        RfsdCreateFcbFromMcb(Vcb, Fcb->RfsdMcb->Parent);
    }

    if (!TargetDcb || !(Fcb->RfsdMcb->Parent->RfsdFcb)) {
        Status = STATUS_UNSUCCESSFUL;

        goto errorout;
    }

    Mcb = NULL;
    Status = RfsdLookupFileName(
                Vcb,
                &FileName,
                TargetMcb,
                &Mcb,
                &Inode ); 

    if (NT_SUCCESS(Status)) {

        if ( (!ReplaceIfExists) ||
             (IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) ||
             (IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_READONLY))) {
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto errorout;
        }

        if (ReplaceIfExists) {
            Status = STATUS_NOT_IMPLEMENTED;
            goto errorout;
        }
    }

    if (IsDirectory(Fcb)) {

        Status = RfsdRemoveEntry( IrpContext, Vcb, 
                                  Fcb->RfsdMcb->Parent->RfsdFcb,
                                  RFSD_FT_DIR,
                                  Fcb->RfsdMcb->Inode );

        if (!NT_SUCCESS(Status)) {
            DbgBreak();

            goto errorout;
        }

        Status = RfsdAddEntry( IrpContext, Vcb, 
                               TargetDcb,
                               RFSD_FT_DIR,
                               Fcb->RfsdMcb->Inode,
                               &FileName );

        if (!NT_SUCCESS(Status)) {

            DbgBreak();

            RfsdAddEntry(  IrpContext, Vcb, 
                           Fcb->RfsdMcb->Parent->RfsdFcb,
                           RFSD_FT_DIR,
                           Fcb->RfsdMcb->Inode,
                           &Fcb->RfsdMcb->ShortName );

            goto errorout;
        }

        if( !RfsdSaveInode( IrpContext,
                            Vcb, 
                            TargetMcb->Inode,
                            TargetDcb->Inode)) {
            Status = STATUS_UNSUCCESSFUL;

            DbgBreak();

            goto errorout;
        }

        if( !RfsdSaveInode( IrpContext,
                            Vcb, 
                            Fcb->RfsdMcb->Parent->Inode,
                            Fcb->RfsdMcb->Parent->RfsdFcb->Inode)) {

            Status = STATUS_UNSUCCESSFUL;

            DbgBreak();

            goto errorout;
        }

        Status = RfsdSetParentEntry( IrpContext, Vcb, Fcb,
                                     Fcb->RfsdMcb->Parent->Inode,
                                     TargetDcb->RfsdMcb->Inode );


        if (!NT_SUCCESS(Status)) {
            DbgBreak();
            goto errorout;
        }

    } else {

        Status = RfsdRemoveEntry( IrpContext, Vcb,
                                  Fcb->RfsdMcb->Parent->RfsdFcb,
                                  RFSD_FT_REG_FILE,
                                  Fcb->RfsdMcb->Inode );
        if (!NT_SUCCESS(Status)) {
            DbgBreak();
            goto errorout;
        }

        Status = RfsdAddEntry( IrpContext,
                               Vcb, TargetDcb,
                               RFSD_FT_REG_FILE,
                               Fcb->RfsdMcb->Inode,
                               &FileName );

        if (!NT_SUCCESS(Status)) {

            DbgBreak();

            RfsdAddEntry(  IrpContext, Vcb, 
                           Fcb->RfsdMcb->Parent->RfsdFcb,
                           RFSD_FT_REG_FILE,
                           Fcb->RfsdMcb->Inode,
                           &Fcb->RfsdMcb->ShortName );

            goto errorout;
        }
    }

    if (NT_SUCCESS(Status)) {

        if (Fcb->RfsdMcb->ShortName.MaximumLength < (FileName.Length + 2)) {

            ExFreePool(Fcb->RfsdMcb->ShortName.Buffer);
            Fcb->RfsdMcb->ShortName.Buffer = 
                ExAllocatePoolWithTag(PagedPool, FileName.Length + 2, RFSD_POOL_TAG);

            if (!Fcb->RfsdMcb->ShortName.Buffer) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }

            Fcb->RfsdMcb->ShortName.MaximumLength = FileName.Length + 2;
        }

        {
            RtlZeroMemory( Fcb->RfsdMcb->ShortName.Buffer,
                           Fcb->RfsdMcb->ShortName.MaximumLength);

            RtlCopyMemory( Fcb->RfsdMcb->ShortName.Buffer,
                           FileName.Buffer, FileName.Length);

            Fcb->RfsdMcb->ShortName.Length = FileName.Length;
        }
    
#if DBG    

        Fcb->AnsiFileName.Length = (USHORT)
            RfsdUnicodeToOEMSize(&FileName) + 1;

        if (Fcb->AnsiFileName.MaximumLength < FileName.Length) {
            ExFreePool(Fcb->AnsiFileName.Buffer);

            Fcb->AnsiFileName.Buffer = 
                ExAllocatePoolWithTag(PagedPool, Fcb->AnsiFileName.Length + 1, RFSD_POOL_TAG);

            if (!Fcb->AnsiFileName.Buffer)  {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }

            RtlZeroMemory( Fcb->AnsiFileName.Buffer, 
                           Fcb->AnsiFileName.Length + 1);
            Fcb->AnsiFileName.MaximumLength = 
                           Fcb->AnsiFileName.Length + 1;
        }

        RfsdUnicodeToOEM( &(Fcb->AnsiFileName),
                          &FileName          );

#endif

        if (bMove) {

            RfsdNotifyReportChange(
                       IrpContext,
                       Vcb,
                       Fcb,
                       (IsDirectory(Fcb) ?
                         FILE_NOTIFY_CHANGE_DIR_NAME :
                         FILE_NOTIFY_CHANGE_FILE_NAME ),
                       FILE_ACTION_REMOVED);

        } else {

            RfsdNotifyReportChange(
                       IrpContext,
                       Vcb,
                       Fcb,
                       (IsDirectory(Fcb) ?
                         FILE_NOTIFY_CHANGE_DIR_NAME :
                         FILE_NOTIFY_CHANGE_FILE_NAME ),
                       FILE_ACTION_RENAMED_OLD_NAME);

        }

        RfsdDeleteMcbNode(Vcb, Fcb->RfsdMcb->Parent, Fcb->RfsdMcb);
        RfsdAddMcbNode(Vcb, TargetMcb, Fcb->RfsdMcb);

        if (bMove) {

            RfsdNotifyReportChange(
                       IrpContext,
                       Vcb,
                       Fcb,
                       (IsDirectory(Fcb) ?
                         FILE_NOTIFY_CHANGE_DIR_NAME :
                         FILE_NOTIFY_CHANGE_FILE_NAME ),
                       FILE_ACTION_ADDED);
        } else {

            RfsdNotifyReportChange(
                       IrpContext,
                       Vcb,
                       Fcb,
                       (IsDirectory(Fcb) ?
                         FILE_NOTIFY_CHANGE_DIR_NAME :
                         FILE_NOTIFY_CHANGE_FILE_NAME ),
                       FILE_ACTION_RENAMED_NEW_NAME  );

        }
    }

errorout:
#endif // 0
    return 0;//Status;
}

NTSTATUS
RfsdDeleteFile(
        PRFSD_IRP_CONTEXT IrpContext,
        PRFSD_VCB Vcb,
        PRFSD_FCB Fcb )
{
    BOOLEAN         bRet = FALSE;
    LARGE_INTEGER   AllocationSize;
    PRFSD_FCB       Dcb = NULL;
    NTSTATUS        Status = STATUS_SUCCESS;

    PAGED_CODE();
#if 0
    RfsdPrint((DBG_INFO, "RfsdDeleteFile: File %S (%xh) will be deleted!\n",
                         Fcb->RfsdMcb->ShortName.Buffer, Fcb->RfsdMcb->Inode));

    if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
        return Status;
    }

    if (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
        if (!RfsdIsDirectoryEmpty(Vcb, Fcb)) {
            ClearFlag(Fcb->Flags, FCB_DELETE_PENDING);
            
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    }

    RfsdPrint((DBG_INFO, "RfsdDeleteFile: RFSDSB->S_FREE_BLOCKS = %xh .\n",
                         Vcb->SuperBlock->s_free_blocks_count));

    if (IsDirectory(Fcb)) {
        if (Fcb->Inode->i_links_count <= 2) {
        } else {
            Status = STATUS_CANNOT_DELETE;
        }
    } else {
        if (Fcb->Inode->i_links_count <= 1) {
        } else {
            Status = STATUS_CANNOT_DELETE;
        }
    }

    if (!NT_SUCCESS(Status)) {
        DbgBreak();
        return Status;
    }

    if (Fcb->RfsdMcb->Parent->RfsdFcb) {

        Status = RfsdRemoveEntry(
                    IrpContext, Vcb, 
                    Fcb->RfsdMcb->Parent->RfsdFcb,
                    (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY) ?
                     RFSD_FT_DIR : RFSD_FT_REG_FILE),
                    Fcb->RfsdMcb->Inode);
    } else {

        Dcb = RfsdCreateFcbFromMcb(Vcb, Fcb->RfsdMcb->Parent);
        if (Dcb) {
            Status = RfsdRemoveEntry(
                        IrpContext, Vcb, Dcb,
                        (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY) ?
                        RFSD_FT_DIR : RFSD_FT_REG_FILE),
                        Fcb->RfsdMcb->Inode);
        }
    }

    if (NT_SUCCESS(Status)) {

        LARGE_INTEGER   SysTime;
        KeQuerySystemTime(&SysTime);

        AllocationSize.QuadPart = (LONGLONG)0;

        Status = RfsdTruncateFile(IrpContext, Vcb, Fcb, &AllocationSize);

        //
        // Update the inode's data length . It should be ZERO if succeeds.
        //

        if (Fcb->Header.FileSize.QuadPart > Fcb->Header.AllocationSize.QuadPart) {

            Fcb->Header.FileSize.QuadPart = Fcb->Header.AllocationSize.QuadPart;
            Fcb->Inode->i_size = Fcb->Header.AllocationSize.QuadPart;
        }
    
        Fcb->Inode->i_links_count = 0;        

        RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, Fcb->Inode);

        if (IsDirectory(Fcb)) {
            bRet = RfsdFreeInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, RFSD_FT_DIR);
        } else {
            bRet = RfsdFreeInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, RFSD_FT_REG_FILE);
        }

        SetFlag(Fcb->Flags, FCB_FILE_DELETED);
        RfsdDeleteMcbNode(Vcb, Fcb->RfsdMcb->Parent, Fcb->RfsdMcb);

    } else {
        DbgBreak();
        RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, Fcb->Inode);
    }

    RfsdPrint((DBG_INFO, "RfsdDeleteFile: Succeed... RFSDSB->S_FREE_BLOCKS = %xh .\n",
                         Vcb->SuperBlock->s_free_blocks_count));
#endif // 0
    return Status;
}

#endif // !RFSD_READ_ONLY

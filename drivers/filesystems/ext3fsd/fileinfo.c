/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fileinfo.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   15 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

VOID
Ext2QueryFileInformationFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PNTSTATUS pStatus,
    IN PEXT2_FCB Fcb,
    IN BOOLEAN FcbResourceAcquired    );

VOID
Ext2SetFileInformationFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PNTSTATUS pStatus,
    IN PEXT2_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN PEXT2_FCB Fcb,
    IN ULONG NotifyFilter,
    IN BOOLEAN VcbMainResourceAcquired,
    IN BOOLEAN FcbMainResourceAcquired,
    IN BOOLEAN FcbPagingIoResourceAcquired,
    IN BOOLEAN CacheInitialized    );

VOID
Ext2DeleteFileFinal (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN PEXT2_FCB Dcb,
    IN PEXT2_FCB Fcb,
    IN BOOLEAN VcbResourceAcquired,
    IN BOOLEAN FcbPagingIoAcquired,
    IN BOOLEAN FcbResourceAcquired,
    IN BOOLEAN DcbResourceAcquired,
    IN BOOLEAN bNewDcb    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2QueryFileInformation)
#pragma alloc_text(PAGE, Ext2SetFileInformation)
#pragma alloc_text(PAGE, Ext2ExpandFile)
#pragma alloc_text(PAGE, Ext2TruncateFile)
#pragma alloc_text(PAGE, Ext2SetDispositionInfo)
#pragma alloc_text(PAGE, Ext2SetRenameInfo)
#pragma alloc_text(PAGE, Ext2DeleteFile)
#endif


/* FUNCTIONS ***************************************************************/


_SEH_DEFINE_LOCALS(Ext2QueryFileInformationFinal)
{
    PEXT2_IRP_CONTEXT IrpContext;
    PNTSTATUS pStatus;
    PEXT2_FCB Fcb;
    BOOLEAN FcbResourceAcquired;
};

_SEH_FINALLYFUNC(Ext2QueryFileInformationFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2QueryFileInformationFinal);
    Ext2QueryFileInformationFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus),
                                  _SEH_VAR(Fcb), _SEH_VAR(FcbResourceAcquired));
}

VOID
Ext2QueryFileInformationFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN PEXT2_FCB            Fcb,
    IN BOOLEAN              FcbResourceAcquired
    )
{
    if (FcbResourceAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }

    if (!IrpContext->ExceptionInProgress) {
        if (*pStatus == STATUS_PENDING ||
            *pStatus == STATUS_CANT_WAIT) {
            *pStatus = Ext2QueueRequest(IrpContext);
        } else {
            Ext2CompleteIrpContext(IrpContext,  *pStatus);
        }
    }
}

NTSTATUS
Ext2QueryFileInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT            FileObject;
    PEXT2_VCB               Vcb;
    PEXT2_FCB               Fcb;
    PEXT2_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    
    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2QueryFileInformationFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(FcbResourceAcquired) = FALSE;

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }
        
        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        _SEH_VAR(Fcb) = Fcb;
        
        if (Fcb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }
        
        if(!((Fcb->Identifier.Type == EXT2FCB) &&
             (Fcb->Identifier.Size == sizeof(EXT2_FCB)))) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        Vcb = Fcb->Vcb;

        {
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)
                )) {

                Status = STATUS_PENDING;
                _SEH_LEAVE;
            }
            
            _SEH_VAR(FcbResourceAcquired) = TRUE;
        }
        
        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb != NULL);
        
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }

                FileBasicInformation = (PFILE_BASIC_INFORMATION) Buffer;

                FileBasicInformation->CreationTime = Fcb->Mcb->CreationTime;
                FileBasicInformation->LastAccessTime = Fcb->Mcb->LastAccessTime;
                FileBasicInformation->LastWriteTime = Fcb->Mcb->LastWriteTime;
                FileBasicInformation->ChangeTime = Fcb->Mcb->ChangeTime;

                FileBasicInformation->FileAttributes = Fcb->Mcb->FileAttr;
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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }
                
                FSI = (PFILE_STANDARD_INFORMATION) Buffer;
                
                FSI->NumberOfLinks = Fcb->Inode->i_links_count;
                
                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                    FSI->DeletePending = FALSE;
                else
                    FSI->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);                
                
                if (IsDirectory(Fcb)) {
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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }
                
                FileInternalInformation = (PFILE_INTERNAL_INFORMATION) Buffer;
                
                /* we use the inode number as the internal index */
                FileInternalInformation->IndexNumber.QuadPart = (LONGLONG)Fcb->Mcb->iNo;
                
                Irp->IoStatus.Information = sizeof(FILE_INTERNAL_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;


            case FileEaInformation:
            {
                PFILE_EA_INFORMATION FileEaInformation;
                
                if (Length < sizeof(FILE_EA_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
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
                
                if (Length < sizeof(FILE_NAME_INFORMATION) +
                    Fcb->Mcb->ShortName.Length - sizeof(WCHAR)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }
                
                FileNameInformation = (PFILE_NAME_INFORMATION) Buffer;
                
                FileNameInformation->FileNameLength = Fcb->Mcb->ShortName.Length;
                
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->Mcb->ShortName.Buffer,
                    Fcb->Mcb->ShortName.Length );
                
                Irp->IoStatus.Information = sizeof(FILE_NAME_INFORMATION) +
                    Fcb->Mcb->ShortName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
            }
            break;
            
            case FilePositionInformation:
            {
                PFILE_POSITION_INFORMATION FilePositionInformation;
                
                if (Length < sizeof(FILE_POSITION_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
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
                
                FileBasicInformation->CreationTime = Fcb->Mcb->CreationTime;
                FileBasicInformation->LastAccessTime = Fcb->Mcb->LastAccessTime;
                FileBasicInformation->LastWriteTime = Fcb->Mcb->LastWriteTime;
                FileBasicInformation->ChangeTime = Fcb->Mcb->ChangeTime;

                FileBasicInformation->FileAttributes = Fcb->Mcb->FileAttr;
                if (FileBasicInformation->FileAttributes == 0) {
                    FileBasicInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                }

                FSI->NumberOfLinks = Fcb->Inode->i_links_count;

                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                    FSI->DeletePending = FALSE;
                else
                    FSI->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);

                if (IsDirectory(Fcb)) {
                    FSI->Directory = TRUE;
                    FSI->AllocationSize.QuadPart = 0;
                    FSI->EndOfFile.QuadPart = 0;
                } else {
                    FSI->Directory = FALSE;
                    FSI->AllocationSize = Fcb->Header.AllocationSize;
                    FSI->EndOfFile = Fcb->Header.FileSize;
                }

                // The "inode number"
                FileInternalInformation->IndexNumber.QuadPart = (LONGLONG)Fcb->Mcb->iNo;

                // Romfs doesn't have any extended attributes
                FileEaInformation->EaSize = 0;

                FilePositionInformation->CurrentByteOffset =
                    FileObject->CurrentByteOffset;
                
                if (Length < sizeof(FILE_ALL_INFORMATION) +
                    Fcb->Mcb->ShortName.Length - sizeof(WCHAR)) {
                    Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    _SEH_LEAVE;
                }
                
                FileNameInformation->FileNameLength = Fcb->Mcb->ShortName.Length;
                
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->Mcb->ShortName.Buffer,
                    Fcb->Mcb->ShortName.Length
                    );
                
                Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION) +
                    Fcb->Mcb->ShortName.Length - sizeof(WCHAR);
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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }

                PFNOI = (PFILE_NETWORK_OPEN_INFORMATION) Buffer;

                PFNOI->FileAttributes = Fcb->Mcb->FileAttr;
                if (PFNOI->FileAttributes == 0) {
                    PFNOI->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                }

                PFNOI->CreationTime   = Fcb->Mcb->CreationTime;
                PFNOI->LastAccessTime = Fcb->Mcb->LastAccessTime;
                PFNOI->LastWriteTime  = Fcb->Mcb->LastWriteTime;
                PFNOI->ChangeTime     = Fcb->Mcb->ChangeTime;

                if (IsDirectory(Fcb)) {
                    PFNOI->AllocationSize.QuadPart = 0;
                    PFNOI->EndOfFile.QuadPart = 0;
                } else {
                    PFNOI->AllocationSize = Fcb->Header.AllocationSize;
                    PFNOI->EndOfFile      = Fcb->Header.FileSize;
                }

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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }
                
                FATI = (PFILE_ATTRIBUTE_TAG_INFORMATION) Buffer;
                
                FATI->FileAttributes = Fcb->Mcb->FileAttr;
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
            DEBUG(DL_USR, ( "Ext2QueryInformation: invalid class: %d\n",
                              FileInformationClass));
            Status = STATUS_INVALID_PARAMETER; /* STATUS_INVALID_INFO_CLASS; */
            break;
        }

    }
    _SEH_FINALLY(Ext2QueryFileInformationFinal_PSEH)
    _SEH_END;

    return Status;
}

_SEH_DEFINE_LOCALS(Ext2SetFileInformationFinal)
{
    PEXT2_IRP_CONTEXT IrpContext;
    PNTSTATUS pStatus;
    PEXT2_VCB Vcb;
    PFILE_OBJECT FileObject;
    PEXT2_FCB Fcb;
    ULONG NotifyFilter;
    BOOLEAN VcbMainResourceAcquired;
    BOOLEAN FcbMainResourceAcquired;
    BOOLEAN FcbPagingIoResourceAcquired;
    BOOLEAN CacheInitialized;
};

_SEH_FINALLYFUNC(Ext2SetFileInformationFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2SetFileInformationFinal);
    Ext2SetFileInformationFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus),
                                _SEH_VAR(Vcb), _SEH_VAR(FileObject), _SEH_VAR(Fcb),
                                _SEH_VAR(NotifyFilter), _SEH_VAR(VcbMainResourceAcquired),
                                _SEH_VAR(FcbMainResourceAcquired),
                                _SEH_VAR(FcbPagingIoResourceAcquired),
                                _SEH_VAR(CacheInitialized));
}

VOID
Ext2SetFileInformationFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN PEXT2_VCB            Vcb,
    IN PFILE_OBJECT         FileObject,
    IN PEXT2_FCB            Fcb,
    IN ULONG                NotifyFilter,
    IN BOOLEAN              VcbMainResourceAcquired,
    IN BOOLEAN              FcbMainResourceAcquired,
    IN BOOLEAN              FcbPagingIoResourceAcquired,
    IN BOOLEAN              CacheInitialized
    )
{
    if (FcbPagingIoResourceAcquired) {
        ExReleaseResourceLite(&Fcb->PagingIoResource);
    }
        
    if (NT_SUCCESS(*pStatus) && (NotifyFilter != 0)) {
        Ext2NotifyReportChange(
                    IrpContext,
                    Vcb,
                    Fcb->Mcb,
                    NotifyFilter,
                    FILE_ACTION_MODIFIED );

    }

    if (FcbMainResourceAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }

    if (CacheInitialized) {
        CcUninitializeCacheMap(FileObject, NULL, NULL);
    }

    if (VcbMainResourceAcquired) {
        ExReleaseResourceLite(&Vcb->MainResource);
    }

    if (!IrpContext->ExceptionInProgress) {
        if (*pStatus == STATUS_PENDING || 
            *pStatus == STATUS_CANT_WAIT ) {
            DbgBreak();
            *pStatus = Ext2QueueRequest(IrpContext);
        } else {
            Ext2CompleteIrpContext(IrpContext,  *pStatus);
        }
    }
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
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;

    ULONG                   Length;
    PVOID                   Buffer;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2SetFileInformationFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(NotifyFilter) = 0;
        _SEH_VAR(VcbMainResourceAcquired) = FALSE;
        _SEH_VAR(FcbMainResourceAcquired) = FALSE;
        _SEH_VAR(FcbPagingIoResourceAcquired) = FALSE;
        _SEH_VAR(CacheInitialized) = FALSE;

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
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
        _SEH_VAR(Vcb) = Vcb;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        if (!IsMounted(Vcb)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }

        /* we need grab Vcb in case it's a rename operation */
        if (FileInformationClass == FileRenameInformation) {
            if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                    Status = STATUS_PENDING;
                    _SEH_LEAVE;
            }
            _SEH_VAR(VcbMainResourceAcquired) = TRUE;
        }

        FileObject = IrpContext->FileObject;
        _SEH_VAR(FileObject) = FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        _SEH_VAR(Fcb) = Fcb;

        // This request is issued to volumes, just return success
        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        ASSERT(Ccb != NULL);
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        
        if ( !IsDirectory(Fcb) && !FlagOn(Fcb->Flags, FCB_PAGE_FILE) &&
             ((FileInformationClass == FileEndOfFileInformation) ||
             (FileInformationClass == FileAllocationInformation))) {

            Status = FsRtlCheckOplock( &Fcb->Oplock,
                                       Irp,
                                       IrpContext,
                                       NULL,
                                       NULL );

            if (Status != STATUS_SUCCESS) {
                _SEH_LEAVE;
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
                _SEH_LEAVE;
            }
            
            _SEH_VAR(FcbMainResourceAcquired) = TRUE;
        }
        
        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {

            if (FileInformationClass != FilePositionInformation) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                _SEH_LEAVE;
            }
        }

        if ( FileInformationClass == FileAllocationInformation ||
             FileInformationClass == FileEndOfFileInformation) {

            if (!ExAcquireResourceExclusiveLite(
                &Fcb->PagingIoResource,
                IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                DbgBreak();
                _SEH_LEAVE;
            }
            
            _SEH_VAR(FcbPagingIoResourceAcquired) = TRUE;
        }
       
/*        
        if (FileInformationClass != FileDispositionInformation 
            && FlagOn(Fcb->Flags, FCB_DELETE_PENDING))
        {
            Status = STATUS_DELETE_PENDING;
            __leave;
        }
*/
        switch (FileInformationClass) {

        case FileBasicInformation:
            {
                PFILE_BASIC_INFORMATION FBI = (PFILE_BASIC_INFORMATION) Buffer;               
                PEXT2_INODE Ext2Inode = Fcb->Inode;

                if (FBI->CreationTime.QuadPart != 0 && FBI->CreationTime.QuadPart != -1) {
                    Ext2Inode->i_ctime = Ext2LinuxTime(FBI->CreationTime);
                    Fcb->Mcb->CreationTime = Ext2NtTime(Ext2Inode->i_ctime);
                    _SEH_VAR(NotifyFilter) |= FILE_NOTIFY_CHANGE_CREATION;
                }

                if (FBI->LastAccessTime.QuadPart != 0 && FBI->LastAccessTime.QuadPart != -1) {
                    Ext2Inode->i_atime = Ext2LinuxTime(FBI->LastAccessTime);
                    Fcb->Mcb->LastAccessTime = Ext2NtTime(Ext2Inode->i_atime);
                    _SEH_VAR(NotifyFilter) |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
                }

                if (FBI->LastWriteTime.QuadPart != 0 && FBI->LastWriteTime.QuadPart != -1) {
                    Ext2Inode->i_mtime = Ext2LinuxTime(FBI->LastWriteTime);
                    Fcb->Mcb->LastWriteTime = Ext2NtTime(Ext2Inode->i_mtime);
                    _SEH_VAR(NotifyFilter) |= FILE_NOTIFY_CHANGE_LAST_WRITE;
                    SetFlag(Ccb->Flags, CCB_LAST_WRITE_UPDATED);
                }

                if (FBI->ChangeTime.QuadPart !=0 && FBI->ChangeTime.QuadPart != -1) {
                    Fcb->Mcb->ChangeTime = FBI->ChangeTime;
                }

                if (FBI->FileAttributes != 0) {

                    BOOLEAN bIsDirectory = IsDirectory(Fcb);
                    _SEH_VAR(NotifyFilter) |= FILE_NOTIFY_CHANGE_ATTRIBUTES;

                    if (IsFlagOn(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY)) {
                        Ext2SetReadOnly(Fcb->Inode->i_mode);
                    } else {
                        Ext2SetOwnerWritable(Fcb->Inode->i_mode);
                    }

                    if (FBI->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                        SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                    } else {
                        ClearFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                    }

                    Fcb->Mcb->FileAttr = FBI->FileAttributes;
                    if (bIsDirectory) {
                        SetFlag(Fcb->Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                    }
                }

                if (_SEH_VAR(NotifyFilter) != 0) {
                    if ( Ext2SaveInode( IrpContext, Vcb, 
                            Fcb->Mcb->iNo, Ext2Inode)) {
                        Status = STATUS_SUCCESS;
                    }
                }

                ClearFlag(_SEH_VAR(NotifyFilter), FILE_NOTIFY_CHANGE_LAST_ACCESS);
                Status = STATUS_SUCCESS;
            }

            break;

        case FileAllocationInformation:
            {
                PFILE_ALLOCATION_INFORMATION FAI = (PFILE_ALLOCATION_INFORMATION)Buffer;
                LARGE_INTEGER RealSize, AllocationSize;

                if (IsDirectory(Fcb)) {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    _SEH_LEAVE;
                } else {
                    Status = STATUS_SUCCESS;
                }

                /* initialize cache map if needed */
                if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
                    (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
                    !IsFlagOn(Irp->Flags, IRP_PAGING_IO)) {

                    ASSERT(!IsFlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE));

                    CcInitializeCacheMap( 
                            FileObject,
                            (PCC_FILE_SIZES)&(Fcb->Header.AllocationSize),
                            FALSE,
                            &(Ext2Global->CacheManagerNoOpCallbacks),
                            Fcb );

                    _SEH_VAR(CacheInitialized) = TRUE;
                }

                /* get user specified allocationsize aligned with BLOCK_SIZE */
                AllocationSize.QuadPart = CEILING_ALIGNED(ULONGLONG, 
                                            (ULONGLONG)FAI->AllocationSize.QuadPart,
                                            (ULONGLONG)BLOCK_SIZE);
                RealSize = Fcb->Mcb->FileSize;

                /* check if file blocks are allocated in Ext2Create */
                if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) {
                    if (Fcb->Header.AllocationSize.QuadPart <
                        Fcb->RealSize.QuadPart) {
                        Fcb->Header.AllocationSize = Fcb->RealSize;
                    } else {
                        ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                    }
                }

                if (AllocationSize.QuadPart > Fcb->Header.AllocationSize.QuadPart) {

                    Status = Ext2ExpandFile(IrpContext, Vcb, Fcb->Mcb, &AllocationSize);
                    Fcb->Header.AllocationSize = AllocationSize;
                    _SEH_VAR(NotifyFilter) = FILE_NOTIFY_CHANGE_SIZE;

                } else if (AllocationSize.QuadPart < Fcb->Header.AllocationSize.QuadPart) {

                    if (MmCanFileBeTruncated(&(Fcb->SectionObject), &AllocationSize))  {

                        /* free blocks from it's real size allocated in Ext2Create */
                        if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) {
                            Fcb->Mcb->FileSize = Fcb->Header.AllocationSize;
                        }

                        /* truncate file blocks */
                        Status = Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb, &AllocationSize);

                        /* restore original file size */
                        if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) { 
                            Fcb->Mcb->FileSize = RealSize;
                            if (NT_SUCCESS(Status)) {
                                ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                            }
                        }

                        _SEH_VAR(NotifyFilter) = FILE_NOTIFY_CHANGE_SIZE;
                        Fcb->Header.AllocationSize.QuadPart = AllocationSize.QuadPart;
                        if (Fcb->Header.FileSize.QuadPart > AllocationSize.QuadPart) {
                            Fcb->Header.FileSize.QuadPart = AllocationSize.QuadPart;
                        }
                        if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart) {
                            Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                        }

                        if (Fcb->Mcb->FileSize.QuadPart > AllocationSize.QuadPart) {
                            Fcb->Mcb->FileSize.QuadPart = AllocationSize.QuadPart;
                            Fcb->Mcb->Inode->i_size = AllocationSize.LowPart;
                            if (S_ISREG(Fcb->Mcb->Inode->i_mode)) {
                                Fcb->Mcb->Inode->i_size_high = (__u32)(AllocationSize.HighPart);
                            }
                        }

                    } else {

                        Status = STATUS_USER_MAPPED_FILE;
                        DbgBreak();
                        _SEH_LEAVE;
                    }
                }

                if (_SEH_VAR(NotifyFilter)) {

                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                    SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                    Ext2SaveInode( IrpContext,
                                   Vcb,
                                   Fcb->Mcb->iNo,
                                   Fcb->Inode );

                    if (CcIsFileCached(FileObject)) {
                        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    }
                }

                DEBUG(DL_INF, ( "Ext2SetInformation: AllocatieonSize = %I64xh Allocated = %I64xh status = %xh\n",
                                       AllocationSize.QuadPart, Fcb->Header.AllocationSize.QuadPart, Status));
            }

            break;

        case FileEndOfFileInformation:
            {
                PFILE_END_OF_FILE_INFORMATION FEOFI = (PFILE_END_OF_FILE_INFORMATION) Buffer;
                LARGE_INTEGER FileSize, AllocationSize, RealSize, EndOfFile;

                if (IsDirectory(Fcb)) {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    _SEH_LEAVE;
                } else {
                    Status = STATUS_SUCCESS;
                }

                /* initialize cache map if needed */
                if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
                    (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
                    !IsFlagOn(Irp->Flags, IRP_PAGING_IO)) {

                    ASSERT(!IsFlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE));

                    CcInitializeCacheMap( 
                            FileObject,
                            (PCC_FILE_SIZES)&(Fcb->Header.AllocationSize),
                            FALSE,
                            &(Ext2Global->CacheManagerNoOpCallbacks),
                            Fcb );

                    _SEH_VAR(CacheInitialized) = TRUE;
                }

                AllocationSize = Fcb->Header.AllocationSize;
                FileSize.QuadPart = CEILING_ALIGNED(ULONGLONG,
                                    FEOFI->EndOfFile.QuadPart, BLOCK_SIZE);
                RealSize = Fcb->Mcb->FileSize;
                EndOfFile = FEOFI->EndOfFile;

                if (IoStackLocation->Parameters.SetFile.AdvanceOnly) {

                    if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
                        _SEH_LEAVE;
                    }

                    if (EndOfFile.QuadPart > Fcb->Header.FileSize.QuadPart) {
                        EndOfFile.QuadPart = Fcb->Header.FileSize.QuadPart;
                    }

                    if (RealSize.QuadPart < EndOfFile.QuadPart) {

                        Fcb->Mcb->FileSize = EndOfFile;
                        Fcb->Inode->i_size = EndOfFile.LowPart;
                        if (S_ISREG(Fcb->Inode->i_mode)) {
                            Fcb->Inode->i_size_high = (__u32)(EndOfFile.HighPart);
                        }
                        _SEH_VAR(NotifyFilter) = FILE_NOTIFY_CHANGE_SIZE;
                        goto FileEndChanged;
                    }

                    _SEH_LEAVE;
                }

                /* check if file blocks are allocated in Ext2Create */
                if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE) &&
                    AllocationSize.QuadPart < Fcb->RealSize.QuadPart) {
                    Fcb->Header.AllocationSize = AllocationSize = Fcb->RealSize;;
                }

                if (FileSize.QuadPart > AllocationSize.QuadPart) {

                    Fcb->Header.AllocationSize = FileSize;
                    Status = Ext2ExpandFile(
                                    IrpContext,
                                    Vcb,
                                    Fcb->Mcb,
                                    &(Fcb->Header.AllocationSize)
                                    );
                    _SEH_VAR(NotifyFilter) = FILE_NOTIFY_CHANGE_SIZE;

                } else if (FileSize.QuadPart == AllocationSize.QuadPart) {

                    /* we are luck ;) */
                    Status = STATUS_SUCCESS;

                } else {

                    /* don't truncate file data since it's still being written */
                    if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_WRITE)) {

                        Status = STATUS_SUCCESS;

                    } else {

                        /* free blocks from it's real size allocated in Ext2Create */
                        if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) {
                            Fcb->Mcb->FileSize = AllocationSize;
                        }

                        /* truncate file blocks */
                        Status = Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb, &FileSize);

                        /* restore original file size */
                        if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) {
                            Fcb->Mcb->FileSize = RealSize;
                            if (NT_SUCCESS(Status)) {
                                ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                            }
                        }

                        /* update file allocateion size */
                        Fcb->Header.AllocationSize.QuadPart = FileSize.QuadPart;

                        if (Fcb->Header.FileSize.QuadPart > FileSize.QuadPart) {
                            Fcb->Header.FileSize.QuadPart = FileSize.QuadPart;
                        }
                        if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart) {
                            Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                        }

                        if (Fcb->Mcb->FileSize.QuadPart > FileSize.QuadPart) {
                            Fcb->Mcb->FileSize.QuadPart = FileSize.QuadPart;
                            Fcb->Mcb->Inode->i_size = FileSize.LowPart;
                            if (S_ISREG(Fcb->Mcb->Inode->i_mode)) {
                                Fcb->Mcb->Inode->i_size_high = (__u32)FileSize.HighPart;
                            }
                        }
                    }

                    _SEH_VAR(NotifyFilter) = FILE_NOTIFY_CHANGE_SIZE;
                }

                if (NT_SUCCESS(Status)) {

                    Fcb->Header.FileSize.QuadPart = Fcb->Mcb->FileSize.QuadPart = EndOfFile.QuadPart;
                    if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart)
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
  
                    Fcb->Inode->i_size = EndOfFile.LowPart;
                    if (S_ISREG(Fcb->Inode->i_mode)) {
                        Fcb->Inode->i_size_high = (__u32)(EndOfFile.HighPart);
                    }

                    if (Fcb->Header.FileSize.QuadPart >= 0x80000000 && 
                        !IsFlagOn(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
                        SetFlag(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE);
                        Ext2SaveSuper(IrpContext, Vcb);
                    }

                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                    SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                    _SEH_VAR(NotifyFilter) = FILE_NOTIFY_CHANGE_SIZE;
                }

FileEndChanged:

                Ext2SaveInode( IrpContext, Vcb, Fcb->Mcb->iNo, Fcb->Inode);

                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                }

                DEBUG(DL_INF, ( "Ext2SetInformation: FileSize = %I64xh RealSize = %I64xh status = %xh\n",
                                      FEOFI->EndOfFile.QuadPart, Fcb->Header.FileSize.QuadPart, Status));
            }

            break;

        case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION FDI = (PFILE_DISPOSITION_INFORMATION)Buffer;
                
                Status = Ext2SetDispositionInfo(IrpContext, Vcb, Fcb, Ccb, FDI->DeleteFile);
                
                DEBUG(DL_INF, ( "Ext2SetInformation: SetDispositionInformation: DeleteFile=%d %wZ status = %xh\n",
                                      FDI->DeleteFile, &Fcb->Mcb->ShortName, Status));
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
                    _SEH_LEAVE;
                }
                
                FilePositionInformation = (PFILE_POSITION_INFORMATION) Buffer;
                
                if ((FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING)) &&
                    (FilePositionInformation->CurrentByteOffset.LowPart &
                    DeviceObject->AlignmentRequirement) ) {
                    Status = STATUS_INVALID_PARAMETER;
                    _SEH_LEAVE;
                }
                
                FileObject->CurrentByteOffset =
                    FilePositionInformation->CurrentByteOffset;
                
                Status = STATUS_SUCCESS;
                _SEH_LEAVE;
            }

            break;

        case FileLinkInformation:

            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            DEBUG(DL_USR, ( "Ext2SetInformation: invalid class: %d\n",
                              FileInformationClass));
            Status = STATUS_INVALID_PARAMETER;/* STATUS_INVALID_INFO_CLASS; */
        }

    }
    _SEH_FINALLY(Ext2SetFileInformationFinal_PSEH)
    _SEH_END;
    
    return Status;
}

NTSTATUS
Ext2ExpandFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG    Layer = 0;
    ULONG    Start = 0;
    ULONG    End = 0;
    ULONG    Extra = 0;
    ULONG    Hint = 0;
    ULONG    Slot = 0;
    ULONG    Base = 0;

    Start = (ULONG)((Mcb->FileSize.QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);
    End = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);

    /* should be a truncate operation */
    if (Start >= End) {
        Size->QuadPart = ((LONGLONG) Start) << BLOCK_BITS;
        return STATUS_SUCCESS;
    }

    /* invalid file size, too big to contain in inode->i_block */
    if (End >= Vcb->MaxInodeBlocks) {
        return STATUS_INVALID_PARAMETER;
    }

    Extra = End - Start;

    for (Layer = 0; Layer < EXT2_BLOCK_TYPES && Extra; Layer++) {

        if (Start >= Vcb->NumBlocks[Layer]) {

            Base  += Vcb->NumBlocks[Layer];
            Start -= Vcb->NumBlocks[Layer];

        } else {

            /* get the slot in i_block array */
            if (Layer == 0) {
                Base = Slot = Start;
            } else {
                Slot = Layer + EXT2_NDIR_BLOCKS - 1;
            }

            /* set block hint to avoid fragments */
            if (Hint == 0) {
                if (Mcb->Inode->i_block[Slot] != 0) {
                    Hint = Mcb->Inode->i_block[Slot];
                } else if (Slot > 1) {
                    Hint = Mcb->Inode->i_block[Slot-1];
                }
            }

            /* now expand this slot */
            Status = Ext2ExpandBlock(
                            IrpContext,
                            Vcb,
                            Mcb,
                            Base,
                            Layer,
                            Start,
                            (Layer == 0) ? (Vcb->NumBlocks[Layer] - Start) : 1,
                            &Mcb->Inode->i_block[Slot],
                            &Hint,
                            &Extra
                        );
            if (!NT_SUCCESS(Status)) {
                break;
            }

            Start = 0;
            if (Layer == 0) {
                Base = 0;
            }
            Base += Vcb->NumBlocks[Layer];
        }
    }

    Size->QuadPart = ((LONGLONG)(End - Extra)) << BLOCK_BITS;

    /* save inode whatever it succeeds to expand or not */
    Ext2SaveInode(
            IrpContext,
            Vcb,
            Mcb->iNo,
            Mcb->Inode
            );

    Ext2SaveInode(IrpContext, Vcb, Mcb->iNo, Mcb->Inode);

    return Status;
}

NTSTATUS
Ext2TruncateFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG    Layer = 0;

    ULONG    Extra = 0;
    ULONG    Wanted = 0;
    ULONG    End;
    ULONG    Base;

    ULONG    SizeArray = 0;
    PULONG   BlockArray = NULL;

    /* translate file size to block */
    End = (ULONG)((Mcb->FileSize.QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);
    Wanted = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);
    Base = Vcb->MaxInodeBlocks;

    /* confrim it's a truncating operation */
    if (Wanted >= End) {
       return Status;
    }

    /* calculate blocks to be freed */
    Extra = End - Wanted;

    for (Layer = EXT2_BLOCK_TYPES; Layer > 0 && Extra; Layer--) {

        if (Vcb->NumBlocks[Layer - 1] == 0) {
            continue;
        }

        Base -= Vcb->NumBlocks[Layer - 1];
        if (Base >= End) {
            continue;
        }

        if (Layer - 1 == 0) {
            BlockArray = &Mcb->Inode->i_block[0];
            SizeArray = End;
        } else {
            BlockArray = &Mcb->Inode->i_block[EXT2_NDIR_BLOCKS - 1 + Layer - 1];
            SizeArray = 1;
        }

        Status = Ext2TruncateBlock(
                        IrpContext,
                        Vcb,
                        Mcb,
                        Base, 
                        End - Base - 1,
                        Layer - 1,
                        SizeArray,
                        BlockArray,
                        &Extra
                    );
        if (!NT_SUCCESS(Status)) {
            break;
        }

        End = Base;
    }

    if (!NT_SUCCESS(Status)) {
        Size->QuadPart += ((ULONGLONG)Extra << BLOCK_BITS);
    }

    if (Size->QuadPart == 0) {
        if (Ext2ListExtents(&Mcb->Extents)) {
            DbgBreak();
        }
        Ext2ClearAllExtents(&Mcb->Extents);
    }

    Mcb->Inode->i_size = Size->LowPart;
    if (S_ISREG(Mcb->Inode->i_mode)) {
        Mcb->Inode->i_size_high = (__u32)(Size->HighPart);
    }

    /* save inode */
    Ext2SaveInode(IrpContext, Vcb, Mcb->iNo, Mcb->Inode);
    
    return Status;
}


NTSTATUS
Ext2IsFileRemovable(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Fcb
    )
{
    if (!MmFlushImageSection( &Fcb->SectionObject,
                              MmFlushForDelete )) {
        return STATUS_CANNOT_DELETE;
    }

    if (Fcb->Mcb->iNo == EXT2_ROOT_INO) {
        return STATUS_CANNOT_DELETE;
    }

    if (IsDirectory(Fcb)) {
        if (!Ext2IsDirectoryEmpty(IrpContext, Vcb, Fcb->Mcb)) {
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    }

    if (IsDirectory(Fcb)) {
        FsRtlNotifyFullChangeDirectory(
                Vcb->NotifySync,
                &Vcb->NotifyList,
                Fcb,
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

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    DEBUG(DL_INF, ( "Ext2SetDispositionInfo: bDelete=%x\n", bDelete));
    
    if (bDelete) {

        DEBUG(DL_INF, ( "Ext2SetDispositionInformation: Removing %wZ.\n", 
                             &Fcb->Mcb->FullName));

        /* always allow deleting on symlinks */
        if (Ccb->SymLink == NULL) {
            status = Ext2IsFileRemovable(IrpContext, Vcb, Fcb);
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

    ULONG                   PrevEntryOffset;
    BOOLEAN                 ReplaceIfExists;
    BOOLEAN                 bMove = FALSE;
    BOOLEAN                 bTargetRemoved = FALSE;

    BOOLEAN                 bNewTargetDcb = FALSE;
    BOOLEAN                 bNewParentDcb = FALSE;

    PFILE_RENAME_INFORMATION    FRI;

    if (Ccb->SymLink) {
        Mcb = Ccb->SymLink;
    }

    if (Mcb->iNo == EXT2_ROOT_INO) {
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
        if (IsMcbSymlink(TargetMcb)) {
            TargetMcb = TargetMcb->Target;
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

    if (TargetMcb->iNo == Mcb->Parent->iNo) {
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

    if ((TargetMcb->iNo != ParentMcb->iNo)) {
        
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

    if (NT_SUCCESS(Status)) {

        if (!ReplaceIfExists) {

            Status = STATUS_OBJECT_NAME_COLLISION;
            DEBUG(DL_RES, ("Ext2SetRenameInfo: Target file %wZ exists\n",
                            &ExistingMcb->FullName));
            Ext2DerefMcb(ExistingMcb);

            goto errorout;

        } else {

            if ( (ExistingFcb = ExistingMcb->Fcb) &&
                 !IsMcbSymlink(ExistingMcb) ) {

                Status = Ext2IsFileRemovable(IrpContext, Vcb, ExistingFcb);
                if (!NT_SUCCESS(Status)) {
                    DEBUG(DL_REN, ("Ext2SetRenameInfo: Target file %wZ cannot be removed.\n",
                                    &ExistingMcb->FullName));
                    Ext2DerefMcb(ExistingMcb);
                    goto errorout;
                }
            }

            Status = Ext2DeleteFile(IrpContext, Vcb, ExistingMcb);
            Ext2DerefMcb(ExistingMcb);
            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to delete %wZ with status: %xh.\n",
                                &FileName, Status));

                goto errorout;
            }

            bTargetRemoved = TRUE;
        }
    }

    PrevEntryOffset = Mcb->EntryOffset;

    if (Ext2InodeType(Mcb) == EXT2_FT_DIR) {

        Status = Ext2AddEntry(
                    IrpContext, Vcb, 
                    TargetDcb,
                    EXT2_FT_DIR,
                    Mcb->iNo,
                    &FileName,
                    &Mcb->EntryOffset
                    );

        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to add entry for %wZ with status: %xh.\n",
                           &FileName, Status));
            goto errorout;
        }

        if (!Ext2SaveInode( IrpContext,
                            Vcb, 
                            TargetMcb->iNo,
                            TargetMcb->Inode)) {

            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to save target Mcb %wZ.\n",
                           &TargetMcb->FullName));
            Status = STATUS_UNSUCCESSFUL;
            DbgBreak();
            goto errorout;
        }

        Status = Ext2RemoveEntry(
                    IrpContext, Vcb, 
                    ParentDcb,
                    PrevEntryOffset,
                    EXT2_FT_DIR,
                    Mcb->iNo
                    );

        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to remove entry %wZ with status %xh.\n",
                           &Mcb->FullName, Status));
            DbgBreak();
            goto errorout;
        }

        if (!Ext2SaveInode( IrpContext,
                            Vcb, 
                            ParentMcb->iNo,
                            ParentMcb->Inode)) {

            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to save parent Mcb %wZ.\n",
                           &ParentMcb->FullName));
            Status = STATUS_UNSUCCESSFUL;
            DbgBreak();
            goto errorout;
        }

        if (IsMcbSymlink(Mcb)) {
            Status = STATUS_SUCCESS;
        } else {
            Status = Ext2SetParentEntry(
                        IrpContext, Vcb, Fcb,
                        ParentMcb->iNo,
                        TargetMcb->iNo );

            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to set parent refer of %wZ with %xh.\n",
                               &Mcb->FullName, Status));
                DbgBreak();
                goto errorout;
            }
        }

    } else {

        Status = Ext2AddEntry( IrpContext,
                               Vcb, TargetDcb,
                               Ext2InodeType(Mcb),
                               Mcb->iNo,
                               &FileName,
                               &Mcb->EntryOffset
                             );

        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to add entry for %wZ with status: %xh.\n",
                           &FileName, Status));
            DbgBreak();
            goto errorout;
        }

        if (!Ext2SaveInode( IrpContext,
                            Vcb, 
                            TargetMcb->iNo,
                            TargetMcb->Inode)) {

            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to save target Mcb %wZ.\n",
                           &TargetMcb->FullName));

            Status = STATUS_UNSUCCESSFUL;
            DbgBreak();
            goto errorout;
        }

        Status = Ext2RemoveEntry( IrpContext, Vcb,
                                  ParentDcb,
                                  PrevEntryOffset,
                                  Ext2InodeType(Mcb),
                                  Mcb->iNo );
        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to remove entry %wZ with status %xh.\n",
                           &Mcb->FullName, Status));
            DbgBreak();
            goto errorout;
        }

        if (!Ext2SaveInode( IrpContext,
                            Vcb, 
                            ParentMcb->iNo,
                            ParentMcb->Inode)) {

            DEBUG(DL_REN, ("Ext2SetRenameInfo: Failed to save parent Mcb %wZ.\n",
                           &ParentMcb->FullName));
            Status = STATUS_UNSUCCESSFUL;
            DbgBreak();
            goto errorout;
        }
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

        if (TargetMcb->iNo != ParentMcb->iNo) {
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

    if (TargetDcb) {
        if (ParentDcb && ParentDcb->Mcb->iNo != TargetDcb->Mcb->iNo) {
            ClearLongFlag(ParentDcb->Flags, FCB_STATE_BUSY);
        }
        ClearLongFlag(TargetDcb->Flags, FCB_STATE_BUSY);
    }

    if (bNewTargetDcb) {
        ASSERT(TargetDcb != NULL);
        if (Ext2DerefXcb(&TargetDcb->ReferenceCount) == 0) {
            Ext2FreeFcb(TargetDcb); TargetDcb = NULL;
        } else {
            DEBUG(DL_RES, ( "Ext2SetRenameInfo: TargetDcb is resued by other threads.\n"));
        }
    }

    if (bNewParentDcb) {
        ASSERT(ParentDcb != NULL);
        if (Ext2DerefXcb(&ParentDcb->ReferenceCount) == 0) {
            Ext2FreeFcb(ParentDcb); ParentDcb = NULL;
        } else {
            DEBUG(DL_RES, ( "Ext2SetRenameInfo: ParentDcb is resued by other threads.\n"));
        }
    }

    return Status;
}

ULONG
Ext2InodeType(PEXT2_MCB Mcb)
{
    if (IsMcbSymlink(Mcb)) {
        return EXT2_FT_SYMLINK;
    }

    if (IsMcbDirectory(Mcb)) {
        return EXT2_FT_DIR;
    }

    return EXT2_FT_REG_FILE;
}

_SEH_DEFINE_LOCALS(Ext2DeleteFileFinal)
{
    PEXT2_VCB Vcb;
    PEXT2_MCB Mcb;
    PEXT2_FCB Dcb;
    PEXT2_FCB Fcb;
    BOOLEAN VcbResourceAcquired;
    BOOLEAN FcbPagingIoAcquired;
    BOOLEAN FcbResourceAcquired;
    BOOLEAN DcbResourceAcquired;
    BOOLEAN bNewDcb;
};

_SEH_FINALLYFUNC(Ext2DeleteFileFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2DeleteFileFinal);
    Ext2DeleteFileFinal(_SEH_VAR(Vcb), _SEH_VAR(Mcb), _SEH_VAR(Dcb),
                        _SEH_VAR(Fcb), _SEH_VAR(VcbResourceAcquired),
                        _SEH_VAR(FcbPagingIoAcquired), _SEH_VAR(FcbResourceAcquired),
                        _SEH_VAR(DcbResourceAcquired), _SEH_VAR(bNewDcb));
}

VOID
Ext2DeleteFileFinal (
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN PEXT2_FCB            Dcb,
    IN PEXT2_FCB            Fcb,
    IN BOOLEAN              VcbResourceAcquired,
    IN BOOLEAN              FcbPagingIoAcquired,
    IN BOOLEAN              FcbResourceAcquired,
    IN BOOLEAN              DcbResourceAcquired,
    IN BOOLEAN              bNewDcb
    )
{
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
}

NTSTATUS
Ext2DeleteFile(
        PEXT2_IRP_CONTEXT IrpContext,
        PEXT2_VCB         Vcb,
        PEXT2_MCB         Mcb
    )
{
    LARGE_INTEGER   Size;
    PEXT2_FCB       Dcb = NULL;
    PEXT2_FCB       Fcb = Mcb->Fcb;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    LARGE_INTEGER   SysTime;

    DEBUG(DL_INF, ( "Ext2DeleteFile: File %wZ (%xh) will be deleted!\n",
                         &Mcb->FullName, Mcb->iNo));

    if (Fcb && IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
        return STATUS_SUCCESS;
    }

    if (!IsMcbSymlink(Mcb) && IsMcbDirectory(Mcb)) {
        if (!Ext2IsDirectoryEmpty(IrpContext, Vcb, Mcb)) {
            if (Fcb) {
                ClearLongFlag(Fcb->Flags, FCB_DELETE_PENDING);
            }
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    }

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2DeleteFileFinal);
        _SEH_VAR(Vcb) = Vcb;
        _SEH_VAR(Mcb) = Mcb;
        _SEH_VAR(Dcb) = Dcb;
        _SEH_VAR(Fcb) = Fcb;
        _SEH_VAR(VcbResourceAcquired) = FALSE;
        _SEH_VAR(FcbPagingIoAcquired) = FALSE;
        _SEH_VAR(FcbResourceAcquired) = FALSE;
        _SEH_VAR(DcbResourceAcquired) = FALSE;
        _SEH_VAR(bNewDcb) = FALSE;

        ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
        _SEH_VAR(VcbResourceAcquired) = TRUE;

        if (!(Dcb = Mcb->Parent->Fcb)) {
            Dcb = Ext2AllocateFcb(Vcb, Mcb->Parent);
            if (Dcb) {
                Ext2ReferXcb(&Dcb->ReferenceCount);
                _SEH_VAR(bNewDcb) = TRUE;
            }
        }

        if (Dcb) {
            SetLongFlag(Dcb->Flags, FCB_STATE_BUSY);
            _SEH_VAR(DcbResourceAcquired) = 
                ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

            /* remove it's entry form it's parent */
            Status = Ext2RemoveEntry(
                        IrpContext,
                        Vcb, Dcb,
                        Mcb->EntryOffset,
                        Ext2InodeType(Mcb),
                        Mcb->iNo );
        }

        if (NT_SUCCESS(Status)) {

            if (Fcb) {
                _SEH_VAR(FcbResourceAcquired) = 
                    ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);

                _SEH_VAR(FcbPagingIoAcquired) = 
                    ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
            }

            /* decrease and check i_links_count */
            Mcb->Inode->i_links_count--;

            if (IsMcbSymlink(Mcb)) {
                if (Mcb->Inode->i_links_count > 0) {
                    Status = STATUS_CANNOT_DELETE;
                }
            } else if (!IsMcbDirectory(Mcb)) {
                if (Mcb->Inode->i_links_count > 0) {
                    goto SkipTruncate;
                }
            } else {
                if (Mcb->Inode->i_links_count >= 2) {
                    goto SkipTruncate;
                }
            }

            if (!NT_SUCCESS(Status)) {
                _SEH_LEAVE;
            }

            if (IsMcbSymlink(Mcb)) {

                /* for symlink, we should do differenctly  */
                if (Mcb->Inode->i_size > EXT2_LINKLEN_IN_INODE) {
                    ASSERT(Mcb->Inode->i_block[0]);
                    ASSERT(Mcb->Inode->i_blocks == (BLOCK_SIZE >> 9));
                    Status = Ext2FreeBlock(IrpContext, Vcb, Mcb->Inode->i_block[0], 1);
                    if (NT_SUCCESS(Status)) {
                        Mcb->Inode->i_block[0] = 0;
                        Mcb->Inode->i_size = 0;
                        Mcb->Inode->i_blocks = 0;
                    }
                }

            } else {

                /* truncate file size */
                Size.QuadPart = (LONGLONG)0;
                Status = Ext2TruncateFile(IrpContext, Vcb, Mcb, &Size);

                /* check file offset mappings */
                DEBUG(DL_EXT, ("Ext2DeleteFile ...: %wZ\n", &Mcb->FullName));
                if (Ext2ListExtents(&Mcb->Extents)) {
                    DbgBreak();
                }
                Ext2ClearAllExtents(&Mcb->Extents);
                ClearLongFlag(Mcb->Flags, MCB_ZONE_INIT);

                /* Update the inode's data length . It should be ZERO if succeeds. */
                if (Fcb) {
                    Fcb->Header.AllocationSize.QuadPart = Size.QuadPart;
                    if (Fcb->Header.FileSize.QuadPart > Size.QuadPart) {
                        Fcb->Header.FileSize.QuadPart = Size.QuadPart;
                    }
                    if (Fcb->Header.ValidDataLength.QuadPart > Fcb->Header.FileSize.QuadPart) {
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                    }
                }

                if (Mcb->FileSize.QuadPart > Size.QuadPart) {
                    Mcb->FileSize.QuadPart = Size.QuadPart;
                    Mcb->Inode->i_size = Size.LowPart;
                    if (S_ISREG(Mcb->Inode->i_mode)) {
                        Mcb->Inode->i_size_high = (__u32)(Size.HighPart);
                    }
                }
            }
    
            /* set delete time and free the inode */
            KeQuerySystemTime(&SysTime);
            Mcb->Inode->i_links_count = 0;
            Mcb->Inode->i_dtime = Ext2LinuxTime(SysTime);
            Ext2FreeInode(IrpContext, Vcb, Mcb->iNo, Ext2InodeType(Mcb));

SkipTruncate:

            Ext2SaveInode(IrpContext, Vcb, Mcb->iNo, Mcb->Inode);

            if (Fcb && !IsMcbSymlink(Mcb)) {
                SetFlag(Fcb->Flags, FCB_FILE_DELETED);
            }

            Ext2RemoveMcb(Vcb, Mcb);
        }

    }
    _SEH_FINALLY(Ext2DeleteFileFinal_PSEH)
    _SEH_END;

    DEBUG(DL_INF, ( "Ext2DeleteFile: %wZ Succeed... EXT2SB->S_FREE_BLOCKS = %xh .\n",
                         &Mcb->FullName, Vcb->SuperBlock->s_free_blocks_count));

    return Status;
}

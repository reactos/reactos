/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             dirctl.c
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

NTSTATUS
RfsdDirectoryCallback(
		    ULONG			BlockNumber,
			PVOID			pContext);

typedef struct _RFSD_CALLBACK_CONTEXT {
	
	PRFSD_VCB					Vcb;	
	PRFSD_CCB					Ccb;

	PRFSD_KEY_IN_MEMORY			pDirectoryKey;
	ULONG						idxStartingDentry;				// The dentry at which the callback should beging triggering output to the Buffer
	ULONG						idxCurrentDentry;				// The current dentry (relative to entire set of dentrys, across all spans)

	// These parameters are forwarded to ProcessDirectoryEntry 
    FILE_INFORMATION_CLASS		FileInformationClass;			// [s]
	PVOID						Buffer;							// [s]
	ULONG						BufferLength;					// [s]
	BOOLEAN						ReturnSingleEntry;				// [s]

    PULONG						pUsedLength;    
	PVOID						pPreviousEntry;    
	
} RFSD_CALLBACK_CONTEXT, *PRFSD_CALLBACK_CONTEXT;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdGetInfoLength)
#pragma alloc_text(PAGE, RfsdProcessDirEntry)
#pragma alloc_text(PAGE, RfsdQueryDirectory)
#pragma alloc_text(PAGE, RfsdNotifyChangeDirectory)
#pragma alloc_text(PAGE, RfsdDirectoryControl)
#pragma alloc_text(PAGE, RfsdIsDirectoryEmpty)
#pragma alloc_text(PAGE, RfsdDirectoryCallback)
#endif

ULONG
RfsdGetInfoLength(IN FILE_INFORMATION_CLASS  FileInformationClass)
{
    PAGED_CODE();

    switch (FileInformationClass) {

    case FileDirectoryInformation:
        return sizeof(FILE_DIRECTORY_INFORMATION);
        break;
        
    case FileFullDirectoryInformation:
        return sizeof(FILE_FULL_DIR_INFORMATION);
        break;
        
    case FileBothDirectoryInformation:
        return sizeof(FILE_BOTH_DIR_INFORMATION);
        break;
        
    case FileNamesInformation:
        return sizeof(FILE_NAMES_INFORMATION);
        break;
        
    default:
        break;
    }

    return 0;
}

ULONG		// Returns 0 on error, or InfoLength + NameLength (the amount of the buffer used for the entry given)
RfsdProcessDirEntry(
			IN PRFSD_VCB         Vcb,
            IN FILE_INFORMATION_CLASS  FileInformationClass,		// Identifier indicating the type of file information this function should report
            IN __u32		 Key_ParentDirectoryID,
			IN __u32		 Key_ObjectID,
            IN PVOID         Buffer,								// The user's buffer, as obtained from the IRP context (it is already gauranteed to be valid)
            IN ULONG         UsedLength,							// Length of Buffer used so far
            IN ULONG         Length,								// Length of Buffer remaining, beyond UsedLength
            IN ULONG         FileIndex,								// Byte offset of the dentry??  (This will just be placed into the file info structure of the same name)
            IN PUNICODE_STRING   pName,								// Filled unicode equivalent of the name (as pulled out from the dentry)
            IN BOOLEAN       Single, 								// Whether or not QueryDirectory is only supposed to return a single entry
			IN PVOID		 pPreviousEntry	)						// A pointer to the previous dir entry in Buffer, which will be linked into the newly added entry if one is created
{
    RFSD_INODE inode;
    PFILE_DIRECTORY_INFORMATION FDI;
    PFILE_FULL_DIR_INFORMATION FFI;
    PFILE_BOTH_DIR_INFORMATION FBI;
    PFILE_NAMES_INFORMATION FNI;

    ULONG InfoLength = 0;
    ULONG NameLength = 0;
    ULONG dwBytes = 0;
    LONGLONG    FileSize;
    LONGLONG    AllocationSize;

    PAGED_CODE();

	// Calculate the size of the entry
    NameLength = pName->Length;
    InfoLength = RfsdGetInfoLength(FileInformationClass);

    if (!InfoLength || InfoLength + NameLength - sizeof(WCHAR) > Length)  {
        RfsdPrint((DBG_INFO, "RfsdPricessDirEntry: Buffer is not enough.\n"));
        return 0;
    }

	// Given the incoming key for this dentry, load the corresponding stat data.
	{ 
	  RFSD_KEY_IN_MEMORY key; 
	  key.k_dir_id = Key_ParentDirectoryID; 
	  key.k_objectid = Key_ObjectID;	  

	  if(!RfsdLoadInode(Vcb, &key, &inode)) {
		  RfsdPrint((DBG_ERROR,  "RfsdPricessDirEntry: Loading stat data %xh, %xh error.\n", Key_ParentDirectoryID, Key_ObjectID));
		  DbgBreak();
		  return 0;
	  }
	}

    FileSize  = (LONGLONG) inode.i_size;
    AllocationSize = CEILING_ALIGNED(FileSize, (ULONGLONG)Vcb->BlockSize);		// TODO: THIS ISN'T QUITE RIGHT

	// Link the previous entry into this entry
	if (pPreviousEntry) {
		// NOTE: All entries begin with NextEntryOffset, so it doesn't matter what type I cast to.
		((PFILE_NAMES_INFORMATION) (pPreviousEntry))->NextEntryOffset = 
			(ULONG) ((PUCHAR) Buffer + UsedLength - (PUCHAR) (pPreviousEntry));
	}

    switch(FileInformationClass) {

    case FileDirectoryInformation:
        FDI = (PFILE_DIRECTORY_INFORMATION) ((PUCHAR)Buffer + UsedLength);
		FDI->NextEntryOffset = 0;

        FDI->FileIndex = FileIndex;
        FDI->CreationTime = RfsdSysTime(inode.i_ctime);
        FDI->LastAccessTime = RfsdSysTime(inode.i_atime);
        FDI->LastWriteTime = RfsdSysTime(inode.i_mtime);
        FDI->ChangeTime = RfsdSysTime(inode.i_mtime);
        FDI->EndOfFile.QuadPart = FileSize;
        FDI->AllocationSize.QuadPart = AllocationSize;
        FDI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        if (FlagOn(Vcb->Flags, VCB_READ_ONLY) || RfsdIsReadOnly(inode.i_mode)) {
            SetFlag(FDI->FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(inode.i_mode))
            FDI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        FDI->FileNameLength = NameLength;
        RtlCopyMemory(FDI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 
        break;
        
    case FileFullDirectoryInformation:

        FFI = (PFILE_FULL_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        FFI->NextEntryOffset = 0;

        FFI->FileIndex = FileIndex;
        FFI->CreationTime = RfsdSysTime(inode.i_ctime);
        FFI->LastAccessTime = RfsdSysTime(inode.i_atime);
        FFI->LastWriteTime = RfsdSysTime(inode.i_mtime);
        FFI->ChangeTime = RfsdSysTime(inode.i_mtime);
        FFI->EndOfFile.QuadPart = FileSize;
        FFI->AllocationSize.QuadPart = AllocationSize;
        FFI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)  || RfsdIsReadOnly(inode.i_mode)) {
            SetFlag(FFI->FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(inode.i_mode))
            FFI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        FFI->FileNameLength = NameLength;
        RtlCopyMemory(FFI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

        break;
        
    case FileBothDirectoryInformation:

        FBI = (PFILE_BOTH_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        FBI->NextEntryOffset = 0;

        FBI->CreationTime = RfsdSysTime(inode.i_ctime);
        FBI->LastAccessTime = RfsdSysTime(inode.i_atime);
        FBI->LastWriteTime = RfsdSysTime(inode.i_mtime);
        FBI->ChangeTime = RfsdSysTime(inode.i_mtime);

        FBI->FileIndex = FileIndex;
        FBI->EndOfFile.QuadPart = FileSize;
        FBI->AllocationSize.QuadPart = AllocationSize;
        FBI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        if (FlagOn(Vcb->Flags, VCB_READ_ONLY)  || RfsdIsReadOnly(inode.i_mode)) {
            SetFlag(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(inode.i_mode))
            FBI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        FBI->FileNameLength = NameLength;
        RtlCopyMemory(FBI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

        break;
        
    case FileNamesInformation:

        FNI = (PFILE_NAMES_INFORMATION) ((PUCHAR)Buffer + UsedLength);        
		FNI->NextEntryOffset = 0;

        FNI->FileNameLength = NameLength;
        RtlCopyMemory(FNI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

        break;
        
    default:
        break;
    }

    return dwBytes;
}

/**
caller suplies a ptr to a file obj for an open target dir, a search pattern to use when listing, and a spec of the info requested.
FSD expected to search and return matching info [503]

The Fcb->RfsdMcb->Key will determine which directory to list the contents of.
*/
__drv_mustHoldCriticalRegion
NTSTATUS
RfsdQueryDirectory (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PRFSD_VCB               Vcb;
    PFILE_OBJECT            FileObject;
    PRFSD_FCB               Fcb = 0;
    PRFSD_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PUNICODE_STRING         FileName;
    ULONG                   FileIndex;
    BOOLEAN                 RestartScan;
    BOOLEAN                 ReturnSingleEntry;
    BOOLEAN                 IndexSpecified;
    PUCHAR                  Buffer;
    BOOLEAN                 FirstQuery;
    PRFSD_KEY_IN_MEMORY		pQueryKey;				// The key of the directory item that is being retrieved
    BOOLEAN                 FcbResourceAcquired = FALSE;
    ULONG                   UsedLength = 0;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
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
		pQueryKey = &(Fcb->RfsdMcb->Key);
        
        ASSERT(Fcb);
        
		KdPrint(("QueryDirectory on Key {%x,%x,%x,%x}\n", 
			pQueryKey->k_dir_id, pQueryKey->k_objectid, pQueryKey->k_offset, pQueryKey->k_type));

        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == RFSDVCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
        
        if (!IsDirectory(Fcb)) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        Ccb = (PRFSD_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb);
        
        ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
            (Ccb->Identifier.Size == sizeof(RFSD_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
#ifndef _GNU_NTIFS_
        
        FileInformationClass =
            IoStackLocation->Parameters.QueryDirectory.FileInformationClass;
        
        Length = IoStackLocation->Parameters.QueryDirectory.Length;
        
        FileName = IoStackLocation->Parameters.QueryDirectory.FileName;
        
        FileIndex = IoStackLocation->Parameters.QueryDirectory.FileIndex;

        RestartScan = FlagOn(IoStackLocation->Flags, SL_RESTART_SCAN);

        ReturnSingleEntry = FlagOn(IoStackLocation->Flags, SL_RETURN_SINGLE_ENTRY);

        IndexSpecified = FlagOn(IoStackLocation->Flags, SL_INDEX_SPECIFIED);
        
#else // _GNU_NTIFS_
        
        FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileInformationClass;
        
        Length = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.Length;
        
        FileName = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileName;
        
        FileIndex = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileIndex;

        RestartScan = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_RESTART_SCAN);

        ReturnSingleEntry = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_RETURN_SINGLE_ENTRY);

        IndexSpecified = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_INDEX_SPECIFIED);
        
#endif // _GNU_NTIFS_
        
/*
        if (!Irp->MdlAddress && Irp->UserBuffer) {
            ProbeForWrite(Irp->UserBuffer, Length, 1);
        }
*/

		// Check that the user's buffer for the output is valid..
        Buffer = RfsdGetUserBuffer(Irp);

        if (Buffer == NULL) {
            DbgBreak();
            Status = STATUS_INVALID_USER_BUFFER;
            _SEH2_LEAVE;
        }
        
		// Check if we have a synchronous or asynchronous request...
        if (!IrpContext->IsSynchronous) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        
        if (!ExAcquireResourceSharedLite(
                 &Fcb->MainResource,
                 IrpContext->IsSynchronous )) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }

        FcbResourceAcquired = TRUE;
        
        if (FileName != NULL) {
			// The caller provided a FileName to search on...

            if (Ccb->DirectorySearchPattern.Buffer != NULL) {
                FirstQuery = FALSE;
            } else {
                FirstQuery = TRUE;
                
				// Set up the DirectorySearchPattern to simply be an uppercase copy of the FileName.
                Ccb->DirectorySearchPattern.Length =
                    Ccb->DirectorySearchPattern.MaximumLength =
                    FileName->Length;
                
                Ccb->DirectorySearchPattern.Buffer =
                    ExAllocatePoolWithTag(PagedPool, FileName->Length, RFSD_POOL_TAG);
                
                if (Ccb->DirectorySearchPattern.Buffer == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH2_LEAVE;
                }

                Status = RtlUpcaseUnicodeString(
                    &(Ccb->DirectorySearchPattern),
                    FileName,
                    FALSE);

                if (!NT_SUCCESS(Status))
                    _SEH2_LEAVE;
            }
        } else if (Ccb->DirectorySearchPattern.Buffer != NULL) {
            FirstQuery = FALSE;
            FileName = &Ccb->DirectorySearchPattern;
        } else {
			// (The FileName and CCB's DirectorySearchPattern were null)

            FirstQuery = TRUE;
            
            Ccb->DirectorySearchPattern.Length =
                Ccb->DirectorySearchPattern.MaximumLength = 2;
            
            Ccb->DirectorySearchPattern.Buffer =
                ExAllocatePoolWithTag(PagedPool, 2, RFSD_POOL_TAG);
            
            if (Ccb->DirectorySearchPattern.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }
            
            RtlCopyMemory(
                Ccb->DirectorySearchPattern.Buffer,
                L"*\0", 2);
        }
        
        if (!IndexSpecified) {
            if (RestartScan || FirstQuery) {
                FileIndex = Fcb->RfsdMcb->DeOffset = 0;
            } else {
				// If we are not starting/restaring a scan, then we must have already started.
				// Retrieve the byte offset (in the directory . file) where we left off.
                FileIndex = Ccb->CurrentByteOffset;
            }
        }
                        
        
        RtlZeroMemory(Buffer, Length);

		// Leave if a previous query has already read the entire contents of the directory
        if (Fcb->Inode->i_size <= FileIndex) {
            Status = STATUS_NO_MORE_FILES;
            _SEH2_LEAVE;
        }               		
        
		//////// 
		
		// Construct a context for the call, and call to parse the entire file system tree.
		// A callback will be triggered on any direntry span belonging to DirectoryKey.
		// This callback will fill the requested section of the user buffer.
		{
			ULONG CurrentFileIndex;

			RFSD_CALLBACK_CONTEXT  CallbackContext;
			CallbackContext.Vcb						= Vcb;
			CallbackContext.Ccb						= Ccb;
			CallbackContext.idxStartingDentry		= FileIndex / sizeof(RFSD_DENTRY_HEAD);
			CallbackContext.idxCurrentDentry		= 0;
			CallbackContext.pDirectoryKey			= pQueryKey;
			CallbackContext.FileInformationClass	= FileInformationClass;
			CallbackContext.Buffer					= Buffer;
			CallbackContext.BufferLength			= Length;
			CallbackContext.ReturnSingleEntry		= ReturnSingleEntry;
			CallbackContext.pUsedLength				= &UsedLength;
			CallbackContext.pPreviousEntry			= NULL;

			RfsdPrint((DBG_TRACE, "Calculated idxCurrentDentry to be %i\n", CallbackContext.idxStartingDentry));

			RfsdParseFilesystemTree(Vcb, pQueryKey, Vcb->SuperBlock->s_root_block, &RfsdDirectoryCallback, &CallbackContext);
		}
				
//================================================================

        if (!UsedLength) {
			// No amount of the dsetination buffer has been used (meaning there were no results)...
            if (FirstQuery) {
                Status = STATUS_NO_SUCH_FILE;
            } else {
                Status = STATUS_NO_MORE_FILES;
            }
        } else {
            Status = STATUS_SUCCESS;
        }

    } _SEH2_FINALLY {
    
        if (FcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }
                       
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                Status = RfsdLockUserBuffer(
                    IrpContext->Irp,
                    Length,
                    IoWriteAccess );
                
                if (NT_SUCCESS(Status)) {
                    Status = RfsdQueueRequest(IrpContext);
                } else {
                    RfsdCompleteIrpContext(IrpContext, Status);
                }
            } else {
                IrpContext->Irp->IoStatus.Information = UsedLength;
                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;
    
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdNotifyChangeDirectory (
    IN PRFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    BOOLEAN             CompleteRequest;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PRFSD_VCB           Vcb;
    PFILE_OBJECT        FileObject;
    PRFSD_FCB           Fcb = 0;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               CompletionFilter;
    BOOLEAN             WatchTree;
    BOOLEAN             bFcbAcquired = FALSE;
    PUNICODE_STRING     FullName;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);

        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
               (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

        //
        //  Always set the wait flag in the Irp context for the original request.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == RfsdGlobal->DeviceObject) {
            CompleteRequest = TRUE;
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

        ASSERT(Fcb);

        if (Fcb->Identifier.Type == RFSDVCB) {
            DbgBreak();  
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
               (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

        if (!IsDirectory(Fcb)) {
            //- DbgBreak();  // NOTE: Windows (at least I've noticed it with the image previewer), will send this request oftentimes on a file!
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                TRUE ))  {
            bFcbAcquired = TRUE;
        } else {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }

        Irp = IrpContext->Irp;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifndef _GNU_NTIFS_

        CompletionFilter =
            IrpSp->Parameters.NotifyDirectory.CompletionFilter;

#else // _GNU_NTIFS_

        CompletionFilter = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.NotifyDirectory.CompletionFilter;

#endif // _GNU_NTIFS_

        WatchTree = IsFlagOn(IrpSp->Flags, SL_WATCH_TREE);

        if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
            Status = STATUS_DELETE_PENDING;
            _SEH2_LEAVE;
        }

        FullName = &Fcb->LongName;

        if (FullName->Buffer == NULL) {
            if (!RfsdGetFullFileName(Fcb->RfsdMcb, FullName)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }
        }

        FsRtlNotifyFullChangeDirectory( Vcb->NotifySync,
                                        &Vcb->NotifyList,
                                        FileObject->FsContext2,
                                        (PSTRING)FullName,
                                        WatchTree,
                                        FALSE,
                                        CompletionFilter,
                                        Irp,
                                        NULL,
                                        NULL );

        CompleteRequest = FALSE;

        Status = STATUS_PENDING;

/*
    Currently the driver is read-only but here is an example on how to use the
    FsRtl-functions to report a change:

    ANSI_STRING TestString;
    USHORT      FileNamePartLength;

    RtlInitAnsiString(&TestString, "\\ntifs.h");

    FileNamePartLength = 7;

    FsRtlNotifyReportChange(
        Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
        &TestString,                // PSTRING      FullTargetName
        &FileNamePartLength,        // PUSHORT      FileNamePartLength
        FILE_NOTIFY_CHANGE_NAME     // ULONG        FilterMatch
        );

    or

    ANSI_STRING TestString;

    RtlInitAnsiString(&TestString, "\\ntifs.h");

    FsRtlNotifyFullReportChange(
        Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
        &TestString,                // PSTRING      FullTargetName
        1,                          // USHORT       TargetNameOffset
        NULL,                       // PSTRING      StreamName OPTIONAL
        NULL,                       // PSTRING      NormalizedParentName OPTIONAL
        FILE_NOTIFY_CHANGE_NAME,    // ULONG        FilterMatch
        0,                          // ULONG        Action
        NULL                        // PVOID        TargetContext
        );
*/

    } _SEH2_FINALLY {

        if (bFcbAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread());
        }

        if (!IrpContext->ExceptionInProgress) {

            if (!CompleteRequest) {
                IrpContext->Irp = NULL;
            }

            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}

VOID
RfsdNotifyReportChange (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB         Vcb,
    IN PRFSD_FCB         Fcb,
    IN ULONG             Filter,
    IN ULONG             Action   )
{
    PUNICODE_STRING FullName;
    USHORT          Offset;

    FullName = &Fcb->LongName;

    // ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    if (FullName->Buffer == NULL) {
        if (!RfsdGetFullFileName(Fcb->RfsdMcb, FullName)) {
            /*Status = STATUS_INSUFFICIENT_RESOURCES;*/
            return;
        }
    }

    Offset = (USHORT) ( FullName->Length - 
                        Fcb->RfsdMcb->ShortName.Length);

    FsRtlNotifyFullReportChange( Vcb->NotifySync,
                                 &(Vcb->NotifyList),
                                 (PSTRING) (FullName),
                                 (USHORT) Offset,
                                 (PSTRING)NULL,
                                 (PSTRING) NULL,
                                 (ULONG) Filter,
                                 (ULONG) Action,
                                 (PVOID) NULL );

    // ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdDirectoryControl (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    switch (IrpContext->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:
        Status = RfsdQueryDirectory(IrpContext);
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        Status = RfsdNotifyChangeDirectory(IrpContext);
        break;
        
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        RfsdCompleteIrpContext(IrpContext, Status);
    }
    
    return Status;
}

#if !RFSD_READ_ONLY

BOOLEAN RfsdIsDirectoryEmpty (
        PRFSD_VCB Vcb,
        PRFSD_FCB Dcb )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    //PRFSD_DIR_ENTRY2        pTarget = NULL;

    ULONG                   dwBytes = 0;
    ULONG                   dwRet;

    BOOLEAN                 bRet = FALSE;

    PAGED_CODE();
#if 0
    if (!IsFlagOn(Dcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }

    _SEH2_TRY {

        pTarget = (PRFSD_DIR_ENTRY2) ExAllocatePoolWithTag(PagedPool,
                                     RFSD_DIR_REC_LEN(RFSD_NAME_LEN), RFSD_POOL_TAG);
        if (!pTarget) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        
        dwBytes = 0;

        bRet = TRUE;

        while ((LONGLONG)dwBytes < Dcb->Header.AllocationSize.QuadPart) {

            RtlZeroMemory(pTarget, RFSD_DIR_REC_LEN(RFSD_NAME_LEN));

            Status = RfsdReadInode(
                        NULL,
                        Vcb,
                        &(Dcb->RfsdMcb->Key),
                        Dcb->Inode,
                        dwBytes,
                        (PVOID)pTarget,
                        RFSD_DIR_REC_LEN(RFSD_NAME_LEN),
                        &dwRet);

            if (!NT_SUCCESS(Status)) {
                RfsdPrint((DBG_ERROR, "RfsdRemoveEntry: Reading Directory Content error.\n"));
                bRet = FALSE;
                _SEH2_LEAVE;
            }

            if (pTarget->inode) {
                if (pTarget->name_len == 1 && pTarget->name[0] == '.') {
                } else if (pTarget->name_len == 2 && pTarget->name[0] == '.' && 
                         pTarget->name[1] == '.') {
                } else {
                    bRet = FALSE;
                    break;
                }
            } else {
                break;
            }

            dwBytes += pTarget->rec_len;
        }

    } _SEH2_FINALLY {

        if (pTarget != NULL) {
            ExFreePool(pTarget);
        }
    } _SEH2_END;
#endif // 0
    return bRet;
}

#endif // !RFSD_READ_ONLY

/**
This callback is triggered when the FS tree parser hits a leaf node that may contain a directory item.
The function then reads each dentry in the item, and is reponsible for sending them into to ProcessDirEntry.
The callback is doing work on behalf of QueryDir -- the context given is from there.
*/
// NOTE: This signature has to be changed here, at the top decleration, and in the RFSD_CALLBACK macro definition
NTSTATUS
RfsdDirectoryCallback(
		    ULONG			BlockNumber,
			PVOID			pContext)
{
	PRFSD_CALLBACK_CONTEXT	pCallbackContext = (PRFSD_CALLBACK_CONTEXT) pContext;
	RFSD_KEY_IN_MEMORY		DirectoryKey;
	PUCHAR					pBlockBuffer			= NULL;
	PRFSD_ITEM_HEAD			pDirectoryItemHeader	= NULL;
	PUCHAR					pDirectoryItemBuffer	= NULL;
	NTSTATUS				Status;
	UNICODE_STRING          InodeFileName;

    PAGED_CODE();

	InodeFileName.Buffer = NULL;

	RfsdPrint((DBG_FUNC, /*__FUNCTION__*/ " invoked on block %i\n", BlockNumber));


	// Load the block
	pBlockBuffer = RfsdAllocateAndLoadBlock(pCallbackContext->Vcb, BlockNumber);
    if (!pBlockBuffer) { Status = STATUS_INSUFFICIENT_RESOURCES; goto out; }	

	// Construct the item key to search for
	DirectoryKey = *(pCallbackContext->pDirectoryKey);
	DirectoryKey.k_type = RFSD_KEY_TYPE_v2_DIRENTRY;

	// Get the item header and its information
	Status = RfsdFindItemHeaderInBlock(
		pCallbackContext->Vcb, &DirectoryKey, pBlockBuffer,
		( &pDirectoryItemHeader ),			//< 
		&CompareKeysWithoutOffset
	); 

	// If this block doesn't happen to contain a directory item, skip it.
	if ( (Status == STATUS_NO_SUCH_MEMBER) || !pDirectoryItemHeader )
	{ 
		KdPrint(("Block %i did not contain the appropriate diritem header\n", BlockNumber));
		Status = STATUS_SUCCESS; goto out; 
	}
	
	RfsdPrint((DBG_INFO, "Found %i dentries in block\n", pDirectoryItemHeader->u.ih_entry_count));

	// Calculate if the requested result will be from this dentry span
	// If the end of this span is not greater than the requested start, it can be skipped.
	if ( !( (pCallbackContext->idxCurrentDentry + (USHORT)(pDirectoryItemHeader->u.ih_entry_count)) > pCallbackContext->idxStartingDentry ) )
	{ 
		RfsdPrint((DBG_TRACE, "SKIPPING block\n"));

		pCallbackContext->idxCurrentDentry += pDirectoryItemHeader->u.ih_entry_count;
		Status = STATUS_SUCCESS;
		goto out;
	}



	// Otherwise, Read out each dentry, and call ProcessDirEntry on it.
	{
		BOOLEAN				bRun = TRUE;
		PRFSD_DENTRY_HEAD	pPrevDentry = NULL;
		ULONG				offsetDentry_toSequentialSpan;			// The byte offset of this dentry, relative to the dentry spans, as though only their DENTRY_HEADs were stitched together sequentially

		// Skip ahead to the starting dentry in this span.
		ULONG	idxDentryInSpan = pCallbackContext->idxStartingDentry - pCallbackContext->idxCurrentDentry;
		pCallbackContext->idxCurrentDentry += idxDentryInSpan;
		
		RfsdPrint((DBG_TRACE, "Sarting dentry: %i.  skipped to %i dentry in span\n", pCallbackContext->idxStartingDentry, idxDentryInSpan));
		
		offsetDentry_toSequentialSpan = pCallbackContext->idxCurrentDentry * sizeof(RFSD_DENTRY_HEAD);

		// Setup the item buffer
		pDirectoryItemBuffer = (PUCHAR) pBlockBuffer + pDirectoryItemHeader->ih_item_location;	


		while (bRun
			&& ( *(pCallbackContext->pUsedLength) < (pCallbackContext->BufferLength) )
			&& (idxDentryInSpan < (pDirectoryItemHeader->u.ih_entry_count)) )
		{

            STRING				OemName;				// FUTURE: does this support codepages?
			PRFSD_DENTRY_HEAD	pCurrentDentry;
			USHORT				InodeFileNameLength = 0;

			// Read a directory entry from the buffered directory item (from the file associated with the filled inode)			
			pCurrentDentry = (PRFSD_DENTRY_HEAD)  (pDirectoryItemBuffer + (idxDentryInSpan * sizeof(RFSD_DENTRY_HEAD) ));			
			
			// Skip the directory entry for the parent of the root directory (because it should not be shown, and has no stat data)
			// (NOTE: Any change made here should also be mirrored in RfsdScanDirCallback)
			if (pCurrentDentry->deh_dir_id == 0 /*&& pCurrentDentry->deh_objectid == 1*/)
			    { goto ProcessNextEntry; }
			
			// Pull the name of the file out from the buffer.
			// NOTE: The filename is not gauranteed to be null-terminated, and so the end may implicitly be the start of the previous entry.
			OemName.Buffer			= (PUCHAR) pDirectoryItemBuffer + pCurrentDentry->deh_location;
			OemName.MaximumLength	= (pPrevDentry ? pPrevDentry->deh_location :			// The end of this entry is the start of the previous
													 pDirectoryItemHeader->ih_item_len		// Otherwise this is the first entry, the end of which is the end of the item.
									  ) -  pCurrentDentry->deh_location;
			if (!pPrevDentry && pCallbackContext->idxStartingDentry > 1 && pCallbackContext->Ccb->deh_location)
			{
				if (OemName.MaximumLength != pCallbackContext->Ccb->deh_location -  pCurrentDentry->deh_location)
				{
					//KdPrint(("Changed MaximumLength from %d to %d for {%.*s}\n", OemName.MaximumLength, pCallbackContext->Ccb->deh_location -  pCurrentDentry->deh_location, RfsdStringLength(OemName.Buffer, pCallbackContext->Ccb->deh_location -  pCurrentDentry->deh_location), OemName.Buffer));
				}
				OemName.MaximumLength = pCallbackContext->Ccb->deh_location -  pCurrentDentry->deh_location;
			}
			OemName.Length			= RfsdStringLength(OemName.Buffer, OemName.MaximumLength);


			// Calculate the name's unicode length, allocate memory, and convert the codepaged name to unicode
            InodeFileNameLength = (USHORT) RfsdOEMToUnicodeSize(&OemName);            
            InodeFileName.Length = 0;
            InodeFileName.MaximumLength = InodeFileNameLength + 2;
            
            if (InodeFileNameLength <= 0) 
				{ break; }

            InodeFileName.Buffer = ExAllocatePoolWithTag(
                PagedPool,
                InodeFileNameLength + 2, RFSD_POOL_TAG);

            if (!InodeFileName.Buffer) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto out;
            }

            RtlZeroMemory(InodeFileName.Buffer, InodeFileNameLength + 2);
            
            Status = RfsdOEMToUnicode( &InodeFileName, &OemName );
			if (!NT_SUCCESS(Status))	{ Status = STATUS_INTERNAL_ERROR; goto out; }		// TODO: CHECK IF TIHS OK            

			////////////// END OF MY PART
            if (FsRtlDoesNameContainWildCards(
                &(pCallbackContext->Ccb->DirectorySearchPattern)) ?
                    FsRtlIsNameInExpression(
                        &(pCallbackContext->Ccb->DirectorySearchPattern),
                        &InodeFileName,
                        TRUE,
                        NULL) :
                !RtlCompareUnicodeString(
                    &(pCallbackContext->Ccb->DirectorySearchPattern),
                    &InodeFileName,
                    TRUE)           ) {
				// The name either contains wild cards, or matches the directory search pattern...
				                
						{
				ULONG dwBytesWritten;
				dwBytesWritten = RfsdProcessDirEntry(
                    pCallbackContext->Vcb, pCallbackContext->FileInformationClass,
                    pCurrentDentry->deh_dir_id,
					pCurrentDentry->deh_objectid,
                    pCallbackContext->Buffer,
                    *(pCallbackContext->pUsedLength),
					pCallbackContext->BufferLength - *(pCallbackContext->pUsedLength),		// The remaining length in the user's buffer
					offsetDentry_toSequentialSpan,
                    &InodeFileName,
                    pCallbackContext->ReturnSingleEntry,
					pCallbackContext->pPreviousEntry);

                if (dwBytesWritten <= 0) {
					Status = STATUS_EVENT_DONE;
                    bRun = FALSE;
					pCallbackContext->Ccb->deh_location = pPrevDentry ? pPrevDentry->deh_location : 0;
                } else {
					pCallbackContext->Ccb->deh_location = 0;
                    pCallbackContext->pPreviousEntry = (PUCHAR) (pCallbackContext->Buffer) + *(pCallbackContext->pUsedLength);
                    *(pCallbackContext->pUsedLength) += dwBytesWritten;
                }
						}
            }
            
            if (InodeFileName.Buffer) {
                ExFreePool(InodeFileName.Buffer);
                InodeFileName.Buffer = NULL;
            }


 ProcessNextEntry:            

			pPrevDentry = pCurrentDentry;

			if (bRun)
			{
				++idxDentryInSpan;				
				++(pCallbackContext->idxCurrentDentry);
				++(pCallbackContext->idxStartingDentry);
				offsetDentry_toSequentialSpan += sizeof(RFSD_DENTRY_HEAD);

				// Store the current position, so that it will be available for the next call
				pCallbackContext->Ccb->CurrentByteOffset = offsetDentry_toSequentialSpan;
			}
			

            if ( ( *(pCallbackContext->pUsedLength) > 0) && pCallbackContext->ReturnSingleEntry) {
				Status = STATUS_EVENT_DONE;
                break;
            }
            
		}

	}

 out:	
	if (pBlockBuffer)			ExFreePool(pBlockBuffer);
	if (InodeFileName.Buffer)	ExFreePool(InodeFileName.Buffer);

	return Status;
}

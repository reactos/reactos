/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             create.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS *****************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

NTSTATUS
RfsdScanDirCallback(
	ULONG		BlockNumber,
	PVOID		pContext );

typedef struct _RFSD_SCANDIR_CALLBACK_CONTEXT {
	IN  PRFSD_VCB				Vcb;
	IN  PRFSD_KEY_IN_MEMORY		pDirectoryKey;
	IN  PUNICODE_STRING			pTargetFilename;

	ULONG idxCurrentDentry;									/// Running count of the dentries processed, so that MatchingIndex will be relative to all dentry spans

	OUT PRFSD_DENTRY_HEAD		pMatchingDentry;			/// If a matching dentry is found, the callback will fill this structure with it
	OUT PULONG					pMatchingIndex;				/// Index of the matching entry (relative to all dentry spans for the directory)
} RFSD_SCANDIR_CALLBACK_CONTEXT, *PRFSD_SCANDIR_CALLBACK_CONTEXT;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdLookupFileName)
#pragma alloc_text(PAGE, RfsdScanDir)
#pragma alloc_text(PAGE, RfsdCreateFile)
#pragma alloc_text(PAGE, RfsdCreateVolume)
#pragma alloc_text(PAGE, RfsdCreate)
#if !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdCreateInode)
#pragma alloc_text(PAGE, RfsdSupersedeOrOverWriteFile)
#endif // !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdScanDirCallback)
#endif

NTSTATUS
RfsdLookupFileName (IN PRFSD_VCB    Vcb,
            IN PUNICODE_STRING      FullFileName,
            IN PRFSD_MCB            ParentMcb,
            OUT PRFSD_MCB *         RfsdMcb,
            IN OUT PRFSD_INODE      Inode)					// An allocated, but unfilled inode, to which the matching file's information will be put
{
    NTSTATUS        Status;
    UNICODE_STRING  FileName;
    PRFSD_MCB       Mcb = 0;

    RFSD_DENTRY_HEAD	DirectoryEntry;
    int             i = 0;
    BOOLEAN         bRun = TRUE;
    BOOLEAN         bParent = FALSE;
    RFSD_INODE      in;
    ULONG           off = 0;

    PAGED_CODE();

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    *RfsdMcb = NULL;


	// Determine the parent node
    if (ParentMcb) {
		// Looking up a file in the tree, starting at an arbitrary parent node.
        bParent = TRUE;
    } else if (FullFileName->Buffer[0] == L'\\') {
		// Looking up from the root (so there was no parent).  Assign the root node parent from the VCB.
        ParentMcb = Vcb->McbTree;
    } else {
		// Otherwise, fail attempt to lookup non-rooted filename
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

	RtlZeroMemory(&DirectoryEntry, sizeof(RFSD_DENTRY_HEAD));

	// Sanity check that the filename is valid
    if (FullFileName->Length == 0) {
        return Status;
    }

	// Only if we're looking up *exactly* the root node, load it, and return it
    if (FullFileName->Length == 2 && FullFileName->Buffer[0] == L'\\') {		
        if (!RfsdLoadInode(Vcb, &(ParentMcb->Key), Inode))  {
            return Status;      
        }

        *RfsdMcb = Vcb->McbTree;

        return STATUS_SUCCESS;
    }

	// Begin lookup from the parent node
    while (bRun && i < FullFileName->Length/2) {
        int Length;
        ULONG FileAttr = FILE_ATTRIBUTE_NORMAL;

        if (bParent) {
            bParent = FALSE;
        } else  {
			// Advance through the (potentially) consecutive '\' path seperators in the filename
            while(i < FullFileName->Length/2 && FullFileName->Buffer[i] == L'\\') i++;
        }

        Length = i;

		// Advance to the next '\' path seperator
        while(i < FullFileName->Length/2 && (FullFileName->Buffer[i] != L'\\')) i++;

		if ( (i - Length) <= 0) {
			// All of the tokens have been parsed...
			break;
		}
		else {
			// There remains a token between the path seperators...
			
			// FileName is a (non-null-terminated) view into the FullFileName structure
            FileName = *FullFileName;
            FileName.Buffer += Length;
            FileName.Length = (USHORT)((i - Length) * 2);

			// Check to see if the parent MCB already contains a child MCB matching the target FileName
            Mcb = RfsdSearchMcb(Vcb, ParentMcb, &FileName);

            if (Mcb) {
                ParentMcb = Mcb;

                Status = STATUS_SUCCESS;

                if (!IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
                    if (i < FullFileName->Length/2) {
                        Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    }

                    break;
                }
            } else {
				// The parent has no child MCB, or there was no child MCB sibling named FileName.  Check the disk using ScanDir...
				
                // Load the parent directory's inode / stat data structure
				// For ReiserFS, I'd need the parent's key.  This has to be a key leading to a directory... I'm just getting it to pass it to scan.				
				if (!RfsdLoadInode(Vcb, &(ParentMcb->Key), &in)) {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }

				// Sanity check that we loaded a directory (unless we loaded the last token, in which case it's ok that we loaded something else??)
                if (!S_ISDIR(in.i_mode)) {
                    if (i < FullFileName->Length/2) {
                        Status =  STATUS_OBJECT_NAME_NOT_FOUND;
                        break;
                    }
                }


                Status = RfsdScanDir (
                            Vcb,
                            ParentMcb,
                            &FileName,
                            &off,			// <
                            &(DirectoryEntry) );		// <

                if (!NT_SUCCESS(Status)) {
					// No such file (or an error occurred), so drop out.
                    bRun = FALSE;
/*
                    if (i >= FullFileName->Length/2)
                    {
                        *RfsdMcb = ParentMcb;
                    }
*/
                } else {
					// We've found what we were looking for...
#if 0			// disabled by ffs too
                    if (IsFlagOn( SUPER_BLOCK->s_feature_incompat, 
                                  RFSD_FEATURE_INCOMPAT_FILETYPE)) {
                        if (rfsd_dir.file_type == RFSD_FT_DIR)
                            SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                    } else 
#endif						  
						{							
						RFSD_KEY_IN_MEMORY key;
						key.k_dir_id = DirectoryEntry.deh_dir_id;
						key.k_objectid = DirectoryEntry.deh_objectid;

                        if (!RfsdLoadInode(Vcb, &key, &in)) {
                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                            break;
                        }
                        if (S_ISDIR(in.i_mode)) {
                            SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                        }
                    }

                    SetFlag(ParentMcb->Flags, MCB_IN_USE);
                    Mcb = RfsdAllocateMcb(Vcb, &FileName, FileAttr);
                    ClearFlag(ParentMcb->Flags, MCB_IN_USE);

                    if (!Mcb) {
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        break;
                    }

                    // NOTE: It should be OK to leave off the 3rd / 4th part of key, because (AFAIK) the only place this is used is ScanDir
					Mcb->Key.k_dir_id = DirectoryEntry.deh_dir_id;  
					Mcb->Key.k_objectid = DirectoryEntry.deh_objectid;
					Mcb->Key.k_offset = Mcb->Key.k_type = 0;
					
                    Mcb->DeOffset = off;
                    RfsdAddMcbNode(Vcb, ParentMcb, Mcb);
                    ParentMcb = Mcb;
                }
            }
        }
    }

    if (NT_SUCCESS(Status)) {
		// If the name has been found, load it according to the inode number in the MCB...
		// The result will be returned to the caller via Inode
        *RfsdMcb = Mcb;
		if (Inode) {
            if (!RfsdLoadInode(Vcb, &(Mcb->Key), Inode)) {
                RfsdPrint((DBG_ERROR, "RfsdLookupFileName: error loading Inode %x,%xh\n",
                          Mcb->Key.k_dir_id, Mcb->Key.k_objectid));
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return Status;
}

/** (This function is only called by LookupFileName.)
 NOTE: The offset and type of the key passed are irrelevant, as the function will always open a a directory for searching, and will search for the contents by file name -- not key.
 
 STATUS_INSUFFICIENT_RESOURCES		if the filename or diskreading buffer could not be allocated
 STATUS_UNSUCCESSFUL				if the buffer could not be read from disk
 STATUS_NO_SUCH_FILE				if the FileName given was not found in the directory scanned
*/
NTSTATUS
RfsdScanDir (IN PRFSD_VCB       Vcb,
         IN PRFSD_MCB           ParentMcb,				// Mcb of the directory to be scanned (which holds the ->Key of the directory)
         IN PUNICODE_STRING     FileName,				// Short file name (not necisarilly null-terminated!)
         IN OUT PULONG          Index,					// Offset (in bytes) of the dentry relative to the start of the directory listing
         IN OUT PRFSD_DENTRY_HEAD rfsd_dir)				// Directory entry of the found item
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	RFSD_KEY_IN_MEMORY		DirectoryKey;

    PAGED_CODE();

    	// Construct the key (for the directory to be searched), by copying the structure
		DirectoryKey = ParentMcb->Key;
		DirectoryKey.k_offset	= 0x1;
		DirectoryKey.k_type		= RFSD_KEY_TYPE_v2_DIRENTRY;

		// Request that the filesystem tree be parsed, looking for FileName in directory spans belonging to DirectoryKey
		{			
			RFSD_SCANDIR_CALLBACK_CONTEXT	CallbackContext;

			CallbackContext.Vcb					= Vcb;
			CallbackContext.pDirectoryKey		= &DirectoryKey;
			CallbackContext.pTargetFilename		= FileName;

			CallbackContext.idxCurrentDentry	= 0;
			
			CallbackContext.pMatchingDentry		= rfsd_dir;
			CallbackContext.pMatchingIndex		= Index;

			Status = RfsdParseFilesystemTree(Vcb, &DirectoryKey, Vcb->SuperBlock->s_root_block, &RfsdScanDirCallback, &CallbackContext);

			if (Status == STATUS_EVENT_DONE)
			{
				Status = STATUS_SUCCESS;
			}
			else if (Status == STATUS_SUCCESS)
			{
				Status = STATUS_NO_SUCH_FILE;
			}
			else
			{
				Status = STATUS_UNSUCCESSFUL;
			}
		}

    RfsdPrint((DBG_TRACE, /*__FUNCTION__*/ " returning %s\n", RfsdNtStatusToString(Status)));
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdCreateFile(PRFSD_IRP_CONTEXT IrpContext, PRFSD_VCB Vcb)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  IrpSp;
    PRFSD_FCB           Fcb = NULL;
    PRFSD_MCB           RfsdMcb = NULL;

    PRFSD_FCB           ParentFcb = NULL;
    PRFSD_MCB           ParentMcb = NULL;

    BOOLEAN             bParentFcbCreated = FALSE;

    PRFSD_CCB           Ccb = NULL;
    PRFSD_INODE         Inode = 0;
    BOOLEAN             VcbResourceAcquired = FALSE;
#if DISABLED
    BOOLEAN             bDir = FALSE;
#endif
    BOOLEAN             bFcbAllocated = FALSE;
    BOOLEAN             bCreated = FALSE;
    UNICODE_STRING      FileName;
    PIRP                Irp;

    ULONG               Options;
    ULONG               CreateDisposition;

    BOOLEAN             OpenDirectory;
    BOOLEAN             OpenTargetDirectory;
    BOOLEAN             CreateDirectory;
    BOOLEAN             SequentialOnly;
    BOOLEAN             NoIntermediateBuffering;
    BOOLEAN             IsPagingFile;
    BOOLEAN             DirectoryFile;
    BOOLEAN             NonDirectoryFile;
    BOOLEAN             NoEaKnowledge;
    BOOLEAN             DeleteOnClose;
    BOOLEAN             TemporaryFile;
    BOOLEAN             CaseSensitive;

    ACCESS_MASK         DesiredAccess;
    ULONG               ShareAccess;

    PAGED_CODE();

    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Options  = IrpSp->Parameters.Create.Options;
    
    DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
    OpenTargetDirectory = IsFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

    NonDirectoryFile = IsFlagOn(Options, FILE_NON_DIRECTORY_FILE);
    SequentialOnly = IsFlagOn(Options, FILE_SEQUENTIAL_ONLY);
    NoIntermediateBuffering = IsFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );
    NoEaKnowledge = IsFlagOn(Options, FILE_NO_EA_KNOWLEDGE);
    DeleteOnClose = IsFlagOn(Options, FILE_DELETE_ON_CLOSE);

    CaseSensitive = IsFlagOn(IrpSp->Flags, SL_CASE_SENSITIVE);

    TemporaryFile = IsFlagOn(IrpSp->Parameters.Create.FileAttributes,
                                   FILE_ATTRIBUTE_TEMPORARY );

    CreateDisposition = (Options >> 24) & 0x000000ff;

    IsPagingFile = IsFlagOn(IrpSp->Flags, SL_OPEN_PAGING_FILE);

    CreateDirectory = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_CREATE) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    OpenDirectory   = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_OPEN) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = IrpSp->Parameters.Create.ShareAccess;

    FileName.Buffer = NULL;

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource, TRUE );
        
        VcbResourceAcquired = TRUE;

        if (Irp->Overlay.AllocationSize.HighPart) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        if (!(Inode = ExAllocatePoolWithTag(
              PagedPool, sizeof(RFSD_INODE), RFSD_POOL_TAG) )) {
            _SEH2_LEAVE;
        }

        RtlZeroMemory(Inode, sizeof(RFSD_INODE));

        FileName.MaximumLength = IrpSp->FileObject->FileName.MaximumLength;
        FileName.Length = IrpSp->FileObject->FileName.Length;

        FileName.Buffer = ExAllocatePoolWithTag(PagedPool, FileName.MaximumLength, RFSD_POOL_TAG);
        if (!FileName.Buffer) {   
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        RtlZeroMemory(FileName.Buffer, FileName.MaximumLength);
        RtlCopyMemory(FileName.Buffer, IrpSp->FileObject->FileName.Buffer, FileName.Length);

        if (IrpSp->FileObject->RelatedFileObject) {
            ParentFcb = (PRFSD_FCB)(IrpSp->FileObject->RelatedFileObject->FsContext);
        }

        if ((FileName.Length > sizeof(WCHAR)) &&
            (FileName.Buffer[1] == L'\\') &&
            (FileName.Buffer[0] == L'\\')) {
            
            FileName.Length -= sizeof(WCHAR);
            
            RtlMoveMemory( &FileName.Buffer[0],
                &FileName.Buffer[1],
                FileName.Length );
            
            //
            //  Bad Name if there are still beginning backslashes.
            //
            
            if ((FileName.Length > sizeof(WCHAR)) &&
                (FileName.Buffer[1] == L'\\') &&
                (FileName.Buffer[0] == L'\\')) {
                                
                Status = STATUS_OBJECT_NAME_INVALID;

                _SEH2_LEAVE;
            }
        }

        if (IsFlagOn(Options, FILE_OPEN_BY_FILE_ID)) {
            Status = STATUS_NOT_IMPLEMENTED;
            _SEH2_LEAVE;
        }

        RfsdPrint((DBG_INFO, "RfsdCreateFile: %S (NameLen=%xh) Paging=%xh Option: %xh.\n",
            FileName.Buffer, FileName.Length, IsPagingFile, IrpSp->Parameters.Create.Options));

        if (ParentFcb) {
            ParentMcb = ParentFcb->RfsdMcb;
        }

        Status = RfsdLookupFileName(
                        Vcb,
                        &FileName,
                        ParentMcb,
                        &RfsdMcb,
                        Inode );

        if (!NT_SUCCESS(Status)) {
            UNICODE_STRING  PathName;
            UNICODE_STRING  RealName;
            UNICODE_STRING  RemainName;

#if DISABLED
            LONG            i = 0;
#endif

            PathName = FileName;

            RfsdPrint((DBG_INFO, "RfsdCreateFile: File %S will be created.\n", PathName.Buffer));

            RfsdMcb = NULL;

            if (PathName.Buffer[PathName.Length/2 - 1] == L'\\') {
                if (DirectoryFile) {
                    PathName.Length -=2;
                    PathName.Buffer[PathName.Length/2] = 0;
                } else {
                    Status = STATUS_NOT_A_DIRECTORY;
                    _SEH2_LEAVE;
                }
            }

            if (!ParentMcb) {
                if (PathName.Buffer[0] != L'\\') {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                } else {
                    ParentMcb = Vcb->McbTree;
                }
            }

Dissecting:

            FsRtlDissectName(PathName, &RealName, &RemainName);

            if (((RemainName.Length != 0) && (RemainName.Buffer[0] == L'\\')) ||
                (RealName.Length >= 256*sizeof(WCHAR))) {
                Status = STATUS_OBJECT_NAME_INVALID;
                _SEH2_LEAVE;
            }

            if (RemainName.Length != 0) {

                PRFSD_MCB   RetMcb;

                Status = RfsdLookupFileName (
                                Vcb,
                                &RealName,
                                ParentMcb,
                                &RetMcb,
                                Inode      );

                if (!NT_SUCCESS(Status)) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                }

                ParentMcb = RetMcb;
                PathName  = RemainName;

                goto Dissecting;
            }

            if (FsRtlDoesNameContainWildCards(&RealName)) {
                Status = STATUS_OBJECT_NAME_INVALID;
                _SEH2_LEAVE;
            }

            ParentFcb = ParentMcb->RfsdFcb;

            if (!ParentFcb) {

                PRFSD_INODE pTmpInode = ExAllocatePoolWithTag(PagedPool, 
                                                        sizeof(RFSD_INODE), RFSD_POOL_TAG);
                if (!pTmpInode) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH2_LEAVE;
                }
				
				if(!RfsdLoadInode(Vcb, &(ParentMcb->Key), pTmpInode)) {
#ifdef __REACTOS__
                    ExFreePool(pTmpInode);
#endif
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                }

                ParentFcb = RfsdAllocateFcb(Vcb,  ParentMcb, pTmpInode);

                if (!ParentFcb) {
                    ExFreePool(pTmpInode);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH2_LEAVE;
                }

                bParentFcbCreated = TRUE;
                ParentFcb->ReferenceCount++;
            }

            // We need to create a new one ?
            if ((CreateDisposition == FILE_CREATE ) ||
                (CreateDisposition == FILE_OPEN_IF) ||
                (CreateDisposition == FILE_OVERWRITE_IF)) {

                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH2_LEAVE;
                }

                if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                    IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                  Vcb->Vpb->RealDevice );
                     SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

                    RfsdRaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                }

                if (DirectoryFile) {
                    if (TemporaryFile) {
                        Status = STATUS_INVALID_PARAMETER;
                        _SEH2_LEAVE;
                    }
                }

                if (!ParentFcb) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                }

                if (DirectoryFile) {
                    if ( RFSD_IS_ROOT_KEY(ParentFcb->RfsdMcb->Key) ) {
                        if ( (RealName.Length == 0x10) && 
                             memcmp(RealName.Buffer, L"Recycled\0", 0x10) == 0) {
                            SetFlag( IrpSp->Parameters.Create.FileAttributes,
                                     FILE_ATTRIBUTE_READONLY );
                        }
                    }

					Status = STATUS_UNSUCCESSFUL;
#if DISABLED
                    Status = RfsdCreateInode( IrpContext,
                                              Vcb, 
                                              ParentFcb,
                                              RFSD_FT_DIR,
                                              IrpSp->Parameters.Create.FileAttributes,
                                              &RealName);
#endif
                } else {
					Status = STATUS_UNSUCCESSFUL;
#if DISABLED
                    Status = RfsdCreateInode( IrpContext,
                                              Vcb,
                                              ParentFcb,
                                              RFSD_FT_REG_FILE,
                                              IrpSp->Parameters.Create.FileAttributes,
                                              &RealName);
#endif
                }
                
                if (NT_SUCCESS(Status)) {

                    bCreated = TRUE;

                    Irp->IoStatus.Information = FILE_CREATED;                    
                    Status = RfsdLookupFileName (
                                Vcb,
                                &RealName,
                                ParentMcb,
                                &RfsdMcb,
                                Inode      );

                    if (NT_SUCCESS(Status)) {

                        if (DirectoryFile) {
DbgBreak();
#if DISABLED		// dirctl.c  [ see also in cleanup.c ]
									RfsdNotifyReportChange(
                                   IrpContext,
                                   Vcb,
                                   ParentFcb,
                                   FILE_NOTIFY_CHANGE_DIR_NAME,
                                   FILE_ACTION_ADDED );
                        } else {
                            RfsdNotifyReportChange(
                                   IrpContext,
                                   Vcb,
                                   ParentFcb,
                                   FILE_NOTIFY_CHANGE_FILE_NAME,
                                   FILE_ACTION_ADDED );
#endif
								}
                    } else {
                        DbgBreak();
                    }
                } else {
                    DbgBreak();
                }
            } else if (OpenTargetDirectory) {
                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH2_LEAVE;
                }

                if (!ParentFcb) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                }

                RtlZeroMemory( IrpSp->FileObject->FileName.Buffer,
                               IrpSp->FileObject->FileName.MaximumLength);
                IrpSp->FileObject->FileName.Length = RealName.Length;

                RtlCopyMemory( IrpSp->FileObject->FileName.Buffer,
                               RealName.Buffer,
                               RealName.Length );

                Fcb = ParentFcb;

                Irp->IoStatus.Information = FILE_DOES_NOT_EXIST;
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                _SEH2_LEAVE;
            }

        } else { // File / Dir already exists.

            if (OpenTargetDirectory) {

                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH2_LEAVE;
                }

                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_SUCCESS;

                RtlZeroMemory( IrpSp->FileObject->FileName.Buffer,
                               IrpSp->FileObject->FileName.MaximumLength);
                IrpSp->FileObject->FileName.Length = RfsdMcb->ShortName.Length;

                RtlCopyMemory( IrpSp->FileObject->FileName.Buffer,
                               RfsdMcb->ShortName.Buffer,
                               RfsdMcb->ShortName.Length );

                //Let Mcb pointer to it's parent
                RfsdMcb = RfsdMcb->Parent;

                goto Openit;
            }

            // We can not create if one exists
            if (CreateDisposition == FILE_CREATE) {
                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_OBJECT_NAME_COLLISION;
                _SEH2_LEAVE;
            }

            if(IsFlagOn(RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    _SEH2_LEAVE;
                }

                if (NonDirectoryFile) {
                    Status = STATUS_FILE_IS_A_DIRECTORY;
                    _SEH2_LEAVE;
                }

                if (RFSD_IS_ROOT_KEY(RfsdMcb->Key)) {

                    if (DeleteOnClose) {
                        Status = STATUS_CANNOT_DELETE;
                        _SEH2_LEAVE;
                    }

                    if (OpenTargetDirectory) {
                        Status = STATUS_INVALID_PARAMETER;
                        _SEH2_LEAVE;
                    }
                }
            }

            Irp->IoStatus.Information = FILE_OPENED;
        }

Openit:
        
        if (RfsdMcb) {

            Fcb = RfsdMcb->RfsdFcb;

            if (!Fcb) {
                Fcb = RfsdAllocateFcb (Vcb, RfsdMcb, Inode);
                bFcbAllocated = TRUE;
            }
        }
        
        if (Fcb) {

            if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
                Status = STATUS_FILE_DELETED;
                _SEH2_LEAVE;
            }

            if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
                Status = STATUS_DELETE_PENDING;
                _SEH2_LEAVE;
            }

            if (bCreated) {
DbgBreak();
#if DISABLED  // ONLY FOR WRITE SUPPORT?

                //
                //  This file is just created.
                //

                if (DirectoryFile) {
                    UNICODE_STRING EntryName;
                    USHORT  NameBuf[6];

                    RtlZeroMemory(&NameBuf, 6 * sizeof(USHORT));

                    EntryName.Length = EntryName.MaximumLength = 2;
                    EntryName.Buffer = &NameBuf[0];
                    NameBuf[0] = (USHORT)'.';

                    RfsdAddEntry( IrpContext, Vcb, Fcb,
                                  RFSD_FT_DIR,
                                  Fcb->RfsdMcb->Key,
                                  &EntryName );

                    RfsdSaveInode( IrpContext, Vcb,
                                   Fcb->RfsdMcb->Key,
                                   Fcb->Inode );

                    EntryName.Length = EntryName.MaximumLength = 4;
                    EntryName.Buffer = &NameBuf[0];
                    NameBuf[0] = NameBuf[1] = (USHORT)'.';

                    RfsdAddEntry( IrpContext, Vcb, Fcb,
                                  RFSD_FT_DIR,
                                  Fcb->RfsdMcb->Parent->Inode,
                                  &EntryName );

                    RfsdSaveInode( IrpContext, Vcb,
                                   Fcb->RfsdMcb->Parent->Inode,
                                   ParentFcb->Inode );
                } else {
DbgBreak();		 
#if DISABLED
                    Status = RfsdExpandFile(
                                IrpContext, Vcb, Fcb,
                                &(Irp->Overlay.AllocationSize));
#endif

                    if (!NT_SUCCESS(Status)) {

                        DbgBreak();

                        _SEH2_LEAVE;
                    }
                }
#endif
            } else {

                //
                //  This file alreayd exists.
                //

                if (DeleteOnClose) {

                    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        _SEH2_LEAVE;
                    }

                    if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;

                        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                      Vcb->Vpb->RealDevice );

                        SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

                        RfsdRaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                    }

                    SetFlag(Fcb->Flags, FCB_DELETE_ON_CLOSE);

                } else {

                    //
                    // Just to Open file (Open/OverWrite ...)
                    //

                    if ((!IsDirectory(Fcb)) && (IsFlagOn(IrpSp->FileObject->Flags,
                                                FO_NO_INTERMEDIATE_BUFFERING))) {
                        Fcb->Header.IsFastIoPossible = FastIoIsPossible;

                        if (IsFlagOn(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED) &&
                            (Fcb->SectionObject.DataSectionObject != NULL)) {

                            if (Fcb->NonCachedOpenCount == Fcb->OpenHandleCount) {
                                /* IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) */

                                if(!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                                    CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
                                    ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                                }

                                CcPurgeCacheSection(&Fcb->SectionObject,
                                                     NULL,
                                                     0,
                                                     FALSE );
                            }
                        }
                    }
                }
            }

            if (!IsDirectory(Fcb)) {
                if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    if ((CreateDisposition == FILE_SUPERSEDE) && !IsPagingFile){
                        DesiredAccess |= DELETE;
                    } else if (((CreateDisposition == FILE_OVERWRITE) ||
                            (CreateDisposition == FILE_OVERWRITE_IF)) && !IsPagingFile) {
                        DesiredAccess |= (FILE_WRITE_DATA | FILE_WRITE_EA |
                                          FILE_WRITE_ATTRIBUTES );
                    }
                }
            }

            if (Fcb->OpenHandleCount > 0) {
                Status = IoCheckShareAccess( DesiredAccess,
                                             ShareAccess,
                                             IrpSp->FileObject,
                                             &(Fcb->ShareAccess),
                                             TRUE );

                if (!NT_SUCCESS(Status)) {
                    _SEH2_LEAVE;
                }
            } else {
                IoSetShareAccess( DesiredAccess,
                                  ShareAccess,
                                  IrpSp->FileObject,
                                  &(Fcb->ShareAccess) );
            }

            Ccb = RfsdAllocateCcb();

            Fcb->OpenHandleCount++;
            Fcb->ReferenceCount++;

            if (IsFlagOn(IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING)) {
                Fcb->NonCachedOpenCount++;
            }

            Vcb->OpenFileHandleCount++;
            Vcb->ReferenceCount++;
            
            IrpSp->FileObject->FsContext = (void*)Fcb;
            IrpSp->FileObject->FsContext2 = (void*) Ccb;
            IrpSp->FileObject->PrivateCacheMap = NULL;
            IrpSp->FileObject->SectionObjectPointer = &(Fcb->SectionObject);
            IrpSp->FileObject->Vpb = Vcb->Vpb;

            Status = STATUS_SUCCESS;

            RfsdPrint((DBG_INFO, "RfsdCreateFile: %s OpenCount: %u ReferCount: %u\n",
                Fcb->AnsiFileName.Buffer, Fcb->OpenHandleCount, Fcb->ReferenceCount));

            if (!IsDirectory(Fcb) && !NoIntermediateBuffering ) {
                IrpSp->FileObject->Flags |= FO_CACHE_SUPPORTED;
            }

            if (!bCreated && !IsDirectory(Fcb)) {
                if ( DeleteOnClose || 
                    IsFlagOn(DesiredAccess, FILE_WRITE_DATA) || 
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {
                    if (!MmFlushImageSection( &Fcb->SectionObject,
                                              MmFlushForWrite )) {

                        Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                                 STATUS_SHARING_VIOLATION;
                        _SEH2_LEAVE;
                    }
                }

                if ((CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {

#if RFSD_READ_ONLY
					Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH2_LEAVE;
#endif


#if DISABLED
                    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        _SEH2_LEAVE;
                    }

                    if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {

                        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                      Vcb->Vpb->RealDevice );
                        SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

                        RfsdRaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                    }

                    Status = RfsdSupersedeOrOverWriteFile( IrpContext,
                                                  Vcb,
                                                  Fcb,
                                                  CreateDisposition );

                    if (NT_SUCCESS(Status)) {
                        _SEH2_LEAVE;
                    }

DbgBreak();
#if DISABLED   
                    Status = RfsdExpandFile(
                                IrpContext,
                                Vcb,
                                Fcb,
                                &(Irp->Overlay.AllocationSize));
#endif

                    if (!(NT_SUCCESS(Status))) {
                        _SEH2_LEAVE;
                    }

DbgBreak();
#if DISABLED   // dirctl.c
                    RfsdNotifyReportChange(
                               IrpContext,
                               Vcb,
                               Fcb,
                               FILE_NOTIFY_CHANGE_LAST_WRITE |
                               FILE_NOTIFY_CHANGE_ATTRIBUTES |
                               FILE_NOTIFY_CHANGE_SIZE,
                               FILE_ACTION_MODIFIED );
#endif

                    if (CreateDisposition == FILE_SUPERSEDE) {
                        Irp->IoStatus.Information = FILE_SUPERSEDED;
                    } else {
                        Irp->IoStatus.Information = FILE_OVERWRITTEN;
                    }
#endif
                }
            }
        }
    } _SEH2_FINALLY {

        if (FileName.Buffer)
            ExFreePool(FileName.Buffer);

        if (bParentFcbCreated) {
            ParentFcb->ReferenceCount--;
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        if (!bFcbAllocated) {
            if (Inode)
                ExFreePool(Inode);
        } else {
            if (!Fcb && Inode)
                ExFreePool(Inode);
        }
    } _SEH2_END;
    
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdCreateVolume(PRFSD_IRP_CONTEXT IrpContext, PRFSD_VCB Vcb)
{
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;

    NTSTATUS            Status;

    ACCESS_MASK         DesiredAccess;
    ULONG               ShareAccess;

    ULONG               Options;
    BOOLEAN             DirectoryFile;
    BOOLEAN             OpenTargetDirectory;

    ULONG               CreateDisposition;

    PAGED_CODE();

	RfsdPrint((DBG_FUNC, "Entering CreateVolume\n"));

    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Options  = IrpSp->Parameters.Create.Options;
    
    DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
    OpenTargetDirectory = IsFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

    CreateDisposition = (Options >> 24) & 0x000000ff;

    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = IrpSp->Parameters.Create.ShareAccess;

    if (DirectoryFile) {
        return STATUS_NOT_A_DIRECTORY;
    }

    if (OpenTargetDirectory) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( (CreateDisposition != FILE_OPEN) && 
         (CreateDisposition != FILE_OPEN_IF) ) {
        return STATUS_ACCESS_DENIED;
    }

    Status = STATUS_SUCCESS;

    if (Vcb->OpenHandleCount > 0) {
        Status = IoCheckShareAccess( DesiredAccess, ShareAccess,
                                     IrpSp->FileObject,
                                     &(Vcb->ShareAccess), TRUE);

        if (!NT_SUCCESS(Status)) {
            goto errorout;
        }
    } else {
        IoSetShareAccess( DesiredAccess, ShareAccess,
                          IrpSp->FileObject,
                          &(Vcb->ShareAccess)   );
    }

    if (FlagOn(DesiredAccess, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA)) {
        ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);

	    RfsdFlushFiles(Vcb, FALSE);
	    RfsdFlushVolume(Vcb, FALSE);

        ExReleaseResourceLite(&Vcb->MainResource);
    }

    {
        PRFSD_CCB   Ccb = RfsdAllocateCcb();

        if (Ccb == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
        IrpSp->FileObject->FsContext  = Vcb;
        IrpSp->FileObject->FsContext2 = Ccb;

        Vcb->ReferenceCount++;
        Vcb->OpenHandleCount++;

        Irp->IoStatus.Information = FILE_OPENED;
    }

errorout:

    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdCreate (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT      DeviceObject;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    PRFSD_VCB           Vcb = 0;
    NTSTATUS            Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PRFSD_FCBVCB        Xcb = NULL;

    PAGED_CODE();

    DeviceObject = IrpContext->DeviceObject;

    Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;

    ASSERT(IsMounted(Vcb));
    
    Irp = IrpContext->Irp;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Xcb = (PRFSD_FCBVCB) (IrpSp->FileObject->FsContext);
    
    if (DeviceObject == RfsdGlobal->DeviceObject) {
        RfsdPrint((DBG_INFO, "RfsdCreate: Create on main device object.\n"));

        Status = STATUS_SUCCESS;
        
        Irp->IoStatus.Information = FILE_OPENED;

        RfsdUnpinRepinnedBcbs(IrpContext);

        RfsdCompleteIrpContext(IrpContext, Status);        

        return Status;
    }
   
    _SEH2_TRY {

        if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            Status = STATUS_ACCESS_DENIED;

            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                Status = STATUS_VOLUME_DISMOUNTED;
            }

            _SEH2_LEAVE;
        }

        if ( ((IrpSp->FileObject->FileName.Length == 0) &&
             (IrpSp->FileObject->RelatedFileObject == NULL)) || 
             (Xcb && Xcb->Identifier.Type == RFSDVCB)  ) {
            Status = RfsdCreateVolume(IrpContext, Vcb);
        } else {
            Status = RfsdCreateFile(IrpContext, Vcb);
        }

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress)  {
            RfsdUnpinRepinnedBcbs(IrpContext);
     
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}

#if !RFSD_READ_ONLY

NTSTATUS
RfsdCreateInode(
    PRFSD_IRP_CONTEXT   IrpContext,
    PRFSD_VCB           Vcb,
    PRFSD_FCB           ParentFcb,
    ULONG               Type,
    ULONG               FileAttr,
    PUNICODE_STRING     FileName)
{
	NTSTATUS    Status;
    ULONG       Inode;
    ULONG       Group;
    RFSD_INODE  RfsdIno;

    PAGED_CODE();
#if 0
    RtlZeroMemory(&RfsdIno, sizeof(RFSD_INODE));

DbgBreak();
#if DISABLED
	 Group = (ParentFcb->RfsdMcb->Inode - 1) / BLOCKS_PER_GROUP;
#endif

    RfsdPrint(( DBG_INFO,
                "RfsdCreateInode: %S in %S(Key=%x,%xh)\n",
                FileName->Buffer, 
                ParentFcb->RfsdMcb->ShortName.Buffer, 
                ParentFcb->RfsdMcb->Key.k_dir_id, ParentFcb->RfsdMcb->Key.k_objectid));

    Status = RfsdNewInode(IrpContext, Vcb, Group,Type, &Inode);

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    Status = RfsdAddEntry(IrpContext, Vcb, ParentFcb, Type, Inode, FileName);

    if (!NT_SUCCESS(Status)) {
        DbgBreak();
        RfsdFreeInode(IrpContext, Vcb, Inode, Type);

        goto errorout;
    }

    RfsdSaveInode(IrpContext, Vcb, ParentFcb->RfsdMcb->Inode, ParentFcb->Inode);

    RfsdIno.i_ctime = ParentFcb->Inode->i_mtime;
    RfsdIno.i_mode =  ( S_IPERMISSION_MASK &
                        ParentFcb->Inode->i_mode );
    RfsdIno.i_uid = ParentFcb->Inode->i_uid;
    RfsdIno.i_gid = ParentFcb->Inode->i_gid;

    //~ RfsdIno.i_dir_acl = ParentFcb->Inode->i_dir_acl;
    //~ RfsdIno.i_file_acl = ParentFcb->Inode->i_file_acl;
    RfsdIno.u.i_generation = ParentFcb->Inode->u.i_generation;

    //~ RfsdIno.osd2 = ParentFcb->Inode->osd2;

    if (IsFlagOn(FileAttr, FILE_ATTRIBUTE_READONLY)) {
        RfsdSetReadOnly(RfsdIno.i_mode);
    }

    if (Type == RFSD_FT_DIR)  {
        RfsdIno.i_mode |= S_IFDIR;
        RfsdIno.i_links_count = 2;
    } else if (Type == RFSD_FT_REG_FILE) {
        RfsdIno.i_mode |= S_IFREG;
        RfsdIno.i_links_count = 1;
    } else {
        DbgBreak();
        RfsdIno.i_links_count = 1;
    }

    RfsdSaveInode(IrpContext, Vcb, Inode, &RfsdIno);

    RfsdPrint((DBG_INFO, "RfsdCreateInode: New Inode = %xh (Type=%xh)\n", Inode, Type));
            
errorout:
#endif // 0
    return 0;//Status;
}

NTSTATUS
RfsdSupersedeOrOverWriteFile(
        PRFSD_IRP_CONTEXT IrpContext,
        PRFSD_VCB Vcb,
        PRFSD_FCB Fcb,
        ULONG     Disposition)
{
    LARGE_INTEGER   CurrentTime;
    LARGE_INTEGER   AllocationSize;
    NTSTATUS        Status = STATUS_SUCCESS;

    PAGED_CODE();
#if 0
    KeQuerySystemTime(&CurrentTime);

    AllocationSize.QuadPart = (LONGLONG)0;

    if (!MmCanFileBeTruncated(&(Fcb->SectionObject), &(AllocationSize))) {
        Status = STATUS_USER_MAPPED_FILE;
        return Status;
    }

DbgBreak();
#if DISABLED
    Status = RfsdTruncateFile(IrpContext, Vcb, Fcb, &AllocationSize);
#endif

    if (NT_SUCCESS(Status)) {			
        Fcb->Header.AllocationSize.QuadPart = 
        Fcb->Header.FileSize.QuadPart =  (LONGLONG) 0;

        Fcb->Inode->i_size = 0;

        if (S_ISREG(Fcb->Inode->i_mode)) {
			KdPrint(("Reminder: Fcb->Inode->i_size_high = 0;\n"));
            //~Fcb->Inode->i_size_high = 0;
        }

        if (Disposition == FILE_SUPERSEDE)
            Fcb->Inode->i_ctime = RfsdInodeTime(CurrentTime);

        Fcb->Inode->i_atime =
        Fcb->Inode->i_mtime = RfsdInodeTime(CurrentTime);
    } else {
        LARGE_INTEGER   iSize;

        iSize.QuadPart  =  (LONGLONG) Fcb->Inode->i_size;

        if (S_ISREG(Fcb->Inode->i_mode))
			KdPrint(("Reminder: Fcb->Inode->i_size_high = 0;\n"));
        //~    iSize.HighPart = (LONG)(Fcb->Inode->i_size_high);

        if (iSize.QuadPart > Fcb->Header.AllocationSize.QuadPart)
            iSize.QuadPart = Fcb->Header.AllocationSize.QuadPart;
    
        Fcb->Header.FileSize.QuadPart =  iSize.QuadPart;

        Fcb->Inode->i_size = iSize.LowPart;
        //~ Fcb->Inode->i_size_high = (ULONG) iSize.HighPart;
    }

    RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, Fcb->Inode);
#endif // 0
    return Status;
}

#endif // !RFSD_READ_ONLY

/**
 Searches to find if the name if is located in the dentry span within the block.

 STATUS_SUCCESS			if the name was not found, but processing should continue
 STATUS_EVENT_DONE		if the name was found, and processing should stop
*/
NTSTATUS
RfsdScanDirCallback(
	ULONG		BlockNumber,
	PVOID		pContext )
{
	PRFSD_SCANDIR_CALLBACK_CONTEXT	pCallbackContext = (PRFSD_SCANDIR_CALLBACK_CONTEXT) pContext;
	RFSD_KEY_IN_MEMORY				DirectoryKey;
	PUCHAR					pBlockBuffer			= NULL;
	PRFSD_ITEM_HEAD			pDirectoryItemHeader	= NULL;
	PUCHAR					pDirectoryItemBuffer	= NULL;
	NTSTATUS				Status;
	BOOLEAN					bFound = FALSE;
	PRFSD_DENTRY_HEAD		pPrevDentry = NULL;
	ULONG					idxDentryInSpan = 0;
	UNICODE_STRING          InodeFileName;
	USHORT                  InodeFileNameLength;

    PAGED_CODE();

	InodeFileName.Buffer = NULL;

	RfsdPrint((DBG_FUNC, /*__FUNCTION__*/ " invoked on block %i\n", BlockNumber));


	_SEH2_TRY {

	// Load the block
	pBlockBuffer = RfsdAllocateAndLoadBlock(pCallbackContext->Vcb, BlockNumber);
    if (!pBlockBuffer) { Status = STATUS_INSUFFICIENT_RESOURCES; _SEH2_LEAVE; }	

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
		Status = STATUS_SUCCESS; _SEH2_LEAVE; 
	}

			// Setup the item buffer
		pDirectoryItemBuffer = (PUCHAR) pBlockBuffer + pDirectoryItemHeader->ih_item_location;	



		// Allocate the unicode filename buffer
	    InodeFileName.Buffer = ExAllocatePoolWithTag(PagedPool, (RFSD_NAME_LEN + 1) * 2, RFSD_POOL_TAG);
		if (!InodeFileName.Buffer) { Status = STATUS_INSUFFICIENT_RESOURCES; _SEH2_LEAVE; }


		while (!bFound && (idxDentryInSpan < pDirectoryItemHeader->u.ih_entry_count) ) {
            OEM_STRING OemName;	

			//
            // reading dir entries from Dcb
            //		
						
			PRFSD_DENTRY_HEAD	pCurrentDentry = (PRFSD_DENTRY_HEAD)  (pDirectoryItemBuffer + (idxDentryInSpan * sizeof(RFSD_DENTRY_HEAD)));

			// Skip the directory entry for the parent of the root directory (because it should not be shown, and has no stat data)
			// (NOTE: Any change made here should also be mirrored in RfsdDirControlCallback)
			if (pCurrentDentry->deh_dir_id == 0 /*&& pCurrentDentry->deh_objectid == 1*/)
			    { goto ProcessNextEntry; }

			// Retrieve the filename of the loaded directory entry from the buffer (encoded with the codepage)            	
			// NOTE: The filename is not gauranteed to be null-terminated, and so the end may implicitly be the start of the previous entry.
			OemName.Buffer			= (PUCHAR) pDirectoryItemBuffer + pCurrentDentry->deh_location;
			OemName.MaximumLength	= (pPrevDentry ? pPrevDentry->deh_location :			// The end of this entry is the start of the previous
													 pDirectoryItemHeader->ih_item_len		// Otherwise this is the first entry, the end of which is the end of the item.
									  ) -  pCurrentDentry->deh_location;
			OemName.Length			= RfsdStringLength(OemName.Buffer, OemName.MaximumLength);



			// Convert that name to unicode
			{
              InodeFileNameLength = (USHORT) RfsdOEMToUnicodeSize(&OemName) + 2;
              
			  // If the unicode InodeFileName.Buffer is not large enough, expand it
			  if (InodeFileName.MaximumLength < InodeFileNameLength)
			  {
				  // Free the existing buffer
				  if (InodeFileName.Buffer)  { ExFreePool(InodeFileName.Buffer); }

				  // Allocate a new larger buffer
				  InodeFileName.Buffer = ExAllocatePoolWithTag(PagedPool, InodeFileNameLength, RFSD_POOL_TAG);
				  if (!InodeFileName.Buffer) { Status = STATUS_INSUFFICIENT_RESOURCES; _SEH2_LEAVE; }
				  InodeFileName.MaximumLength = InodeFileNameLength;				  
			  }

              InodeFileName.Length = 0;
              
              RtlZeroMemory( InodeFileName.Buffer, InodeFileNameLength);
              
              Status = RfsdOEMToUnicode(
                              &InodeFileName,
                              &OemName    );

              if (!NT_SUCCESS(Status)) { _SEH2_LEAVE; }
			}


			// Compare it to the name we are searching for
            if (!RtlCompareUnicodeString(
				pCallbackContext->pTargetFilename,
                    &InodeFileName,
                    TRUE ))  {
				// This entry MATCHED!  Copy the matching dentry into the output field on the context
                bFound = TRUE;
				
				*(pCallbackContext->pMatchingIndex) = (pCallbackContext->idxCurrentDentry * sizeof(RFSD_DENTRY_HEAD));
                RtlCopyMemory(pCallbackContext->pMatchingDentry, pCurrentDentry, sizeof(RFSD_DENTRY_HEAD));				
                
                RfsdPrint(( DBG_INFO, /*__FUNCTION__*/ ": Found: Name=%S Key=%xh,%xh\n",
                            InodeFileName.Buffer, pCurrentDentry->deh_dir_id, pCurrentDentry->deh_objectid ));

				Status = STATUS_EVENT_DONE;
				break;
            }
            
		  ProcessNextEntry:
			// Advance to the next directory entry
			pPrevDentry = pCurrentDentry;
            ++idxDentryInSpan;
			++(pCallbackContext->idxCurrentDentry);
        }

        if (!bFound) {
			// Indicate success, so that parsing will continue with subsequent blocks.
            Status = STATUS_SUCCESS;
        }
	} _SEH2_FINALLY {
		if (pBlockBuffer)			ExFreePool(pBlockBuffer);
		if (InodeFileName.Buffer)	ExFreePool(InodeFileName.Buffer);
	} _SEH2_END;


	return Status;

}

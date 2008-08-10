/*************************************************************************
*
* File: create.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "Create"/"Open" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_CREATE

#define			DEBUG_LEVEL						(DEBUG_TRACE_CREATE)
 

/*************************************************************************
*
* Function: Ext2Create()
*
* Description: 
*	The I/O Manager will invoke this routine to handle a create/open
*	request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*	to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2Create(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
    PtrExt2IrpContext	PtrIrpContext;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace( DEBUG_TRACE_IRP_ENTRY, "Create Control IRP received...", 0);

	FsRtlEnterFileSystem();
	
	//	Ext2BreakPoint();

	ASSERT(DeviceObject);
	ASSERT(Irp);

	// sometimes, we may be called here with the device object representing
	//	the file system (instead of the device object created for a logical
	//	volume. In this case, there is not much we wish to do (this create
	//	typically will happen 'cause some process has to open the FSD device
	//	object so as to be able to send an IOCTL to the FSD)

	//	All of the logical volume device objects we create have a device
	//	extension whereas the device object representing the FSD has no
	//	device extension. This seems like a good enough method to identify
	//	between the two device objects ...
	if (DeviceObject->Size == (unsigned short)(sizeof(DEVICE_OBJECT))) 
	{
		// this is an open of the FSD itself
		DebugTrace( DEBUG_TRACE_MISC, " === Open for the FSD itself", 0);
		Irp->IoStatus.Status = RC;
		Irp->IoStatus.Information = FILE_OPENED;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return(RC);
	}

	// set the top level context
	AreWeTopLevel = Ext2IsIrpTopLevel(Irp);

	try 
	{

		// get an IRP context structure and issue the request
		PtrIrpContext = Ext2AllocateIrpContext(Irp, DeviceObject);
		ASSERT(PtrIrpContext);

		RC = Ext2CommonCreate(PtrIrpContext, Irp, TRUE );

	} 
	except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
	{

		RC = Ext2ExceptionHandler(PtrIrpContext, Irp);

		Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
	}

	if (AreWeTopLevel) 
	{
		IoSetTopLevelIrp(NULL);
	}
	
	FsRtlExitFileSystem();

	return(RC);
}



/*************************************************************************
*
* Function: Ext2CommonCreate()
*
* Description:
*	The actual work is performed here. This routine may be invoked in one'
*	of the two possible contexts:
*	(a) in the context of a system worker thread
*	(b) in the context of the original caller
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2CommonCreate(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
BOOLEAN						FirstAttempt)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION		PtrIoStackLocation = NULL;
	PIO_SECURITY_CONTEXT	PtrSecurityContext = NULL;
	PFILE_OBJECT			PtrNewFileObject = NULL;
	PFILE_OBJECT			PtrRelatedFileObject = NULL;
	uint32					AllocationSize = 0; 	// if we create a new file
	PFILE_FULL_EA_INFORMATION	PtrExtAttrBuffer = NULL;
	unsigned long			RequestedOptions = 0;
	unsigned long			RequestedDisposition = 0;
	uint8					FileAttributes = 0;
	unsigned short			ShareAccess = 0;
	unsigned long			ExtAttrLength = 0;
	ACCESS_MASK				DesiredAccess;

	BOOLEAN					DeferredProcessing = FALSE;

	PtrExt2VCB				PtrVCB = NULL;
	BOOLEAN					AcquiredVCB = FALSE;

	BOOLEAN					DirectoryOnlyRequested = FALSE;
	BOOLEAN					FileOnlyRequested = FALSE;
	BOOLEAN					NoBufferingSpecified = FALSE;
	BOOLEAN					WriteThroughRequested = FALSE;
	BOOLEAN					DeleteOnCloseSpecified = FALSE;
	BOOLEAN					NoExtAttrKnowledge = FALSE;
	BOOLEAN					CreateTreeConnection = FALSE;
	BOOLEAN					OpenByFileId	= FALSE;

	BOOLEAN					SequentialOnly	= FALSE;
	BOOLEAN					RandomAccess	= FALSE;
	
	// Are we dealing with a page file?
	BOOLEAN					PageFileManipulation = FALSE;

	// Is this open for a target directory (used in rename operations)?
	BOOLEAN					OpenTargetDirectory = FALSE;

	// Should we ignore case when attempting to locate the object?
	BOOLEAN					IgnoreCaseWhenChecking = FALSE;

	PtrExt2CCB				PtrRelatedCCB = NULL, PtrNewCCB = NULL;
	PtrExt2FCB				PtrRelatedFCB = NULL, PtrNewFCB = NULL;

	unsigned long			ReturnedInformation = -1;

	UNICODE_STRING			TargetObjectName;
	UNICODE_STRING			RelatedObjectName;

	UNICODE_STRING			AbsolutePathName;
	UNICODE_STRING			RenameLinkTargetFileName;


	ASSERT(PtrIrpContext);
	ASSERT(PtrIrp);

	try 
	{

		AbsolutePathName.Buffer = NULL;
		AbsolutePathName.Length = AbsolutePathName.MaximumLength = 0;

		// Getting a pointer to the current I/O stack location
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

		// Can we block?
		if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK)) 
		{
			//	Asynchronous processing required...
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
			DeferredProcessing = TRUE;
			try_return(RC);
		}

		// Obtaining the parameters specified by the user.
		PtrNewFileObject	= PtrIoStackLocation->FileObject;
		TargetObjectName	= PtrNewFileObject->FileName;
		PtrRelatedFileObject = PtrNewFileObject->RelatedFileObject;

		if( PtrNewFileObject->FileName.Length && PtrNewFileObject->FileName.Buffer )
		{
			if( PtrNewFileObject->FileName.Buffer[ PtrNewFileObject->FileName.Length/2 ] != 0 )
			{
				DebugTrace(DEBUG_TRACE_MISC, "&&&&&&&&&  PtrFileObject->FileName not NULL terminated! [Create]", 0 );
			}
			DebugTrace( DEBUG_TRACE_FILE_NAME, " === Create/Open File Name : -%S- [Create]", PtrNewFileObject->FileName.Buffer );
		}
		else
		{
			DebugTrace( DEBUG_TRACE_FILE_NAME, " === Create/Open File Name : -null- [Create]", 0);
		}

		// Is this a Relative Create/Open?
		if (PtrRelatedFileObject) 
		{
			PtrRelatedCCB = (PtrExt2CCB)(PtrRelatedFileObject->FsContext2);
			ASSERT(PtrRelatedCCB);
			ASSERT(PtrRelatedCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_CCB);
			// each CCB in turn points to a FCB
			PtrRelatedFCB = PtrRelatedCCB->PtrFCB;
			ASSERT(PtrRelatedFCB);
			if( PtrRelatedFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB &&
				PtrRelatedFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_VCB	)
			{
				//	How the hell can this happen!!!
				Ext2BreakPoint();
			}

			AssertFCBorVCB( PtrRelatedFCB );

			RelatedObjectName = PtrRelatedFileObject->FileName;

			if( PtrRelatedFileObject->FileName.Length && PtrRelatedFileObject->FileName.Buffer )
			{
				DebugTrace( DEBUG_TRACE_FILE_NAME, " === Relative to : -%S-", PtrRelatedFileObject->FileName.Buffer );
			}
			else
			{
				DebugTrace( DEBUG_TRACE_FILE_NAME, " === Relative to : -null-",0);
			}

		}


		AllocationSize    = PtrIrp->Overlay.AllocationSize.LowPart;
		//	Only 32 bit file sizes supported...

		if (PtrIrp->Overlay.AllocationSize.HighPart) 
		{
			RC = STATUS_INVALID_PARAMETER;
			try_return(RC);
		}

		// Getting a pointer to the supplied security context
		PtrSecurityContext = PtrIoStackLocation->Parameters.Create.SecurityContext;

		// Obtaining the desired access 
		DesiredAccess = PtrSecurityContext->DesiredAccess;

		//	Getting the options supplied by the user...
		RequestedOptions = (PtrIoStackLocation->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS);
		RequestedDisposition = ((PtrIoStackLocation->Parameters.Create.Options >> 24) & 0xFF);

		FileAttributes	= (uint8)(PtrIoStackLocation->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS);
		ShareAccess	= PtrIoStackLocation->Parameters.Create.ShareAccess;
		PtrExtAttrBuffer	= PtrIrp->AssociatedIrp.SystemBuffer;

		ExtAttrLength		= PtrIoStackLocation->Parameters.Create.EaLength;

		SequentialOnly      = ((RequestedOptions & FILE_SEQUENTIAL_ONLY ) ? TRUE : FALSE);
		RandomAccess		= ((RequestedOptions & FILE_RANDOM_ACCESS ) ? TRUE : FALSE);
		

		DirectoryOnlyRequested = ((RequestedOptions & FILE_DIRECTORY_FILE) ? TRUE : FALSE);
		FileOnlyRequested = ((RequestedOptions & FILE_NON_DIRECTORY_FILE) ? TRUE : FALSE);
		NoBufferingSpecified = ((RequestedOptions & FILE_NO_INTERMEDIATE_BUFFERING) ? TRUE : FALSE);
		WriteThroughRequested = ((RequestedOptions & FILE_WRITE_THROUGH) ? TRUE : FALSE);
		DeleteOnCloseSpecified = ((RequestedOptions & FILE_DELETE_ON_CLOSE) ? TRUE : FALSE);
		NoExtAttrKnowledge = ((RequestedOptions & FILE_NO_EA_KNOWLEDGE) ? TRUE : FALSE);
		CreateTreeConnection = ((RequestedOptions & FILE_CREATE_TREE_CONNECTION) ? TRUE : FALSE);
		OpenByFileId = ((RequestedOptions & FILE_OPEN_BY_FILE_ID) ? TRUE : FALSE);
		PageFileManipulation = ((PtrIoStackLocation->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE);
		OpenTargetDirectory = ((PtrIoStackLocation->Flags & SL_OPEN_TARGET_DIRECTORY) ? TRUE : FALSE);
		IgnoreCaseWhenChecking = ((PtrIoStackLocation->Flags & SL_CASE_SENSITIVE) ? TRUE : FALSE);

		// Ensure that the operation has been directed to a valid VCB ...
		PtrVCB =	(PtrExt2VCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
		ASSERT(PtrVCB);
		ASSERT(PtrVCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);

		
		if( !PtrNewFileObject->Vpb )
		{
			PtrNewFileObject->Vpb = PtrVCB->PtrVPB;
		}

		//	Acquiring the VCBResource Exclusively...
		//	This is done to synchronise with the close and cleanup routines...
		
		DebugTrace(DEBUG_TRACE_MISC,  "*** Going into a block to acquire VCB Exclusively [Create]", 0);
		
		DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Create]", PtrVCB->VCBResource.ActiveCount, PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
		ExAcquireResourceExclusiveLite(&(PtrVCB->VCBResource), TRUE);
				
		AcquiredVCB = TRUE;

		DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Acquired in Create", 0);
		if( PtrNewFileObject )
		{
			DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Create]", PtrNewFileObject);
		}

		//	Verify Volume...
		//	if (!NT_SUCCESS(RC = Ext2VerifyVolume(PtrVCB))) 
		//	{
		//		try_return(RC);
		//	}

		// If the volume has been locked, fail the request

		if (PtrVCB->VCBFlags & EXT2_VCB_FLAGS_VOLUME_LOCKED) 
		{
			DebugTrace(DEBUG_TRACE_MISC,   "Volume locked. Failing Create", 0 );
			RC = STATUS_ACCESS_DENIED;
			try_return(RC);
		}


		if ((PtrNewFileObject->FileName.Length == 0) && ((PtrRelatedFileObject == NULL) ||
			  (PtrRelatedFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB))) 
		{
			//
			//	>>>>>>>>>>>>>	Volume Open requested. <<<<<<<<<<<<<
			//

			//	Performing validity checks...
			if ((OpenTargetDirectory) || (PtrExtAttrBuffer)) 
			{
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}

			if (DirectoryOnlyRequested) 
			{
				// a volume is not a directory
				RC = STATUS_NOT_A_DIRECTORY;
				try_return(RC);
			}

			if ((RequestedDisposition != FILE_OPEN) && (RequestedDisposition != FILE_OPEN_IF)) 
			{
				// cannot create a new volume, I'm afraid ...
				RC = STATUS_ACCESS_DENIED;
				try_return(RC);
			}
			DebugTrace(DEBUG_TRACE_MISC,   "Volume open requested", 0 );
			RC = Ext2OpenVolume(PtrVCB, PtrIrpContext, PtrIrp, ShareAccess, PtrSecurityContext, PtrNewFileObject);
			ReturnedInformation = PtrIrp->IoStatus.Information;

			try_return(RC);
		}

		if (OpenByFileId) 
		{
			DebugTrace(DEBUG_TRACE_MISC, "Open by File Id requested", 0 );
			RC = STATUS_ACCESS_DENIED;
			try_return(RC);
		}

		// Relative path name specified...
		if (PtrRelatedFileObject)
		{

			if (!(PtrRelatedFCB->FCBFlags & EXT2_FCB_DIRECTORY)) 
			{
				// we must have a directory as the "related" object
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}

			//	Performing validity checks...
			if ((RelatedObjectName.Length == 0) || (RelatedObjectName.Buffer[0] != L'\\')) 
			{
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}

			if ((TargetObjectName.Length != 0) && (TargetObjectName.Buffer[0] == L'\\')) 
			{
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}

			// Creating an absolute path-name.
			{
				AbsolutePathName.MaximumLength = TargetObjectName.Length + RelatedObjectName.Length + sizeof(WCHAR);
				if (!(AbsolutePathName.Buffer = Ext2AllocatePool(PagedPool, AbsolutePathName.MaximumLength ))) 
				{
					RC = STATUS_INSUFFICIENT_RESOURCES;
					try_return(RC);
				}

				RtlZeroMemory(AbsolutePathName.Buffer, AbsolutePathName.MaximumLength);

				RtlCopyMemory((void *)(AbsolutePathName.Buffer), (void *)(RelatedObjectName.Buffer), RelatedObjectName.Length);
				AbsolutePathName.Length = RelatedObjectName.Length;
				RtlAppendUnicodeToString(&AbsolutePathName, L"\\");
				RtlAppendUnicodeToString(&AbsolutePathName, TargetObjectName.Buffer);
			}

		}
		//	Absolute Path name specified...
		else 
		{
			

			// Validity Checks...
			if (TargetObjectName.Buffer[0] != L'\\') 
			{
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}

			{
				AbsolutePathName.MaximumLength = TargetObjectName.Length;
				if (!(AbsolutePathName.Buffer = Ext2AllocatePool(PagedPool, AbsolutePathName.MaximumLength ))) {
					RC = STATUS_INSUFFICIENT_RESOURCES;
					try_return(RC);
				}

				RtlZeroMemory(AbsolutePathName.Buffer, AbsolutePathName.MaximumLength);

				RtlCopyMemory((void *)(AbsolutePathName.Buffer), (void *)(TargetObjectName.Buffer), TargetObjectName.Length);
				AbsolutePathName.Length = TargetObjectName.Length;
			}
		}


		//	Parsing the path...
		if (AbsolutePathName.Length == 2) 
		{
			
			// this is an open of the root directory, ensure that	the caller has not requested a file only
			if (FileOnlyRequested || (RequestedDisposition == FILE_SUPERSEDE) || (RequestedDisposition == FILE_OVERWRITE) ||
				 (RequestedDisposition == FILE_OVERWRITE_IF)) 
			{
				RC = STATUS_FILE_IS_A_DIRECTORY;
				try_return(RC);
			}

			RC = Ext2OpenRootDirectory(PtrVCB, PtrIrpContext, PtrIrp, ShareAccess, PtrSecurityContext, PtrNewFileObject);
			DebugTrace(DEBUG_TRACE_MISC,   " === Root directory opened", 0 );
			try_return(RC);
		}


		{
			//	Used during parsing the file path...
			UNICODE_STRING			RemainingName;
			UNICODE_STRING			CurrentName;
			UNICODE_STRING			NextRemainingName;
			ULONG					CurrInodeNo = 0;
			PtrExt2FCB				PtrCurrFCB = NULL;
			PtrExt2FCB				PtrNextFCB = NULL;
			PFILE_OBJECT			PtrCurrFileObject = NULL;
			ULONG					Type = 0;
			LARGE_INTEGER ZeroSize;
			BOOLEAN Found		= FALSE;
			
			ZeroSize.QuadPart = 0;
			if ( PtrRelatedFileObject ) 
			{
				CurrInodeNo = PtrRelatedFCB->INodeNo;
				PtrCurrFCB = PtrRelatedFCB;
			}
			else 
			{
				CurrInodeNo = PtrVCB->PtrRootDirectoryFCB->INodeNo;
				PtrCurrFCB = PtrVCB->PtrRootDirectoryFCB;

			}

			//	Ext2ZerooutUnicodeString( &RemainingName );
			Ext2ZerooutUnicodeString( &CurrentName );
			Ext2ZerooutUnicodeString( &NextRemainingName );

			RemainingName = TargetObjectName;
		
			while ( !Found && CurrInodeNo ) 
			{
				FsRtlDissectName ( RemainingName, &CurrentName, &NextRemainingName );

				RemainingName = NextRemainingName;
				//	CurrInodeNo is the parent inode for the entry I am searching for
				//	PtrCurrFCB	is the parent's FCB
				//	Current Name is its name...


				PtrNextFCB = Ext2LocateChildFCBInCore ( PtrVCB, &CurrentName, CurrInodeNo );
				
				if( PtrNextFCB )
				{
					CurrInodeNo = PtrNextFCB->INodeNo;
	
					if( NextRemainingName.Length == 0 )
					{
						//
						//	Done Parsing...
						//	Found the file...
						//
						Found = TRUE;
						
						if( OpenTargetDirectory )
						{
							//
							//	This is for a rename/move operation...
							//
							ReturnedInformation = FILE_EXISTS;

							//	Now replace the file name field with that of the 
							//	Target file name...
							Ext2CopyUnicodeString( 
								&RenameLinkTargetFileName,
								&CurrentName );
							/*
							
							for( i = 0; i < (CurrentName.Length/2); i++ )
							{
								PtrNewFileObject->FileName.Buffer[i] = CurrentName.Buffer[i];
							}
							PtrNewFileObject->FileName.Length = CurrentName.Length;
							*/
							//	Now open the Parent Directory...
							PtrNextFCB = PtrCurrFCB;
							CurrInodeNo = PtrNextFCB->INodeNo;
						}

						//	
						//	Relating the FCB to the New File Object
						//
						PtrNewFileObject->Vpb = PtrVCB->PtrVPB;
						PtrNewFileObject->PrivateCacheMap = NULL;
						PtrNewFileObject->FsContext = (void *)( &(PtrNextFCB->NTRequiredFCB.CommonFCBHeader) );
						PtrNewFileObject->SectionObjectPointer = &(PtrNextFCB->NTRequiredFCB.SectionObject) ;
						break;
					}

					else if( !Ext2IsFlagOn( PtrNextFCB->FCBFlags, EXT2_FCB_DIRECTORY ) )
					{
						//	Invalid path...
						//	Can have only a directory in the middle of the path...
						//
						RC = STATUS_OBJECT_PATH_NOT_FOUND;
						try_return( RC );
					}
				}
				else	//	searching on the disk...
				{
					CurrInodeNo = Ext2LocateFileInDisk( PtrVCB, &CurrentName, PtrCurrFCB, &Type );
					if( !CurrInodeNo )
					{
						//
						//	Not found...
						//	Quit searching...
						//	
					
						if( ( NextRemainingName.Length == 0 ) && (
							( RequestedDisposition == FILE_CREATE ) ||
							( RequestedDisposition == FILE_OPEN_IF) ||
							( RequestedDisposition == FILE_OVERWRITE_IF) ) )

						{
							//
							//	Just the last component was not found...
							//	A create was requested...
							//
							if( DirectoryOnlyRequested )
							{
								Type = EXT2_FT_DIR;
							}
							else
							{
								Type = EXT2_FT_REG_FILE;
							}

							CurrInodeNo = Ext2CreateFile( PtrIrpContext, PtrVCB, 
								&CurrentName, PtrCurrFCB, Type );
							
							if(	!CurrInodeNo )
							{
								RC = STATUS_OBJECT_PATH_NOT_FOUND;
								try_return( RC );
							}
							// Set the allocation size for the object is specified
							//IoSetShareAccess(DesiredAccess, ShareAccess, PtrNewFileObject, &(PtrNewFCB->FCBShareAccess));
							//	RC = STATUS_SUCCESS;
							ReturnedInformation = FILE_CREATED;
							
							//	Should also create a CCB structure...
							//	Doing that a little fathre down... ;)

						}
						else if( NextRemainingName.Length == 0 && OpenTargetDirectory )
						{ 
							//
							//	This is for a rename/move operation...
							//	Just the last component was not found...
							//
							ReturnedInformation = FILE_DOES_NOT_EXIST;

							//	Now replace the file name field with that of the 
							//	Target file name...
							Ext2CopyUnicodeString( 
								&RenameLinkTargetFileName,
								&CurrentName );
							/*
							for( i = 0; i < (CurrentName.Length/2); i++ )
							{
								PtrNewFileObject->FileName.Buffer[i] = CurrentName.Buffer[i];
							}
							PtrNewFileObject->FileName.Length = CurrentName.Length;
							*/

							//	Now open the Parent Directory...
							PtrNextFCB = PtrCurrFCB;
							CurrInodeNo = PtrNextFCB->INodeNo;
							//	Initialize the FsContext
							PtrNewFileObject->FsContext = &PtrNextFCB->NTRequiredFCB.CommonFCBHeader;
							//	Initialize the section object pointer...
							PtrNewFileObject->SectionObjectPointer = &(PtrNextFCB->NTRequiredFCB.SectionObject);
							PtrNewFileObject->Vpb = PtrVCB->PtrVPB;
							PtrNewFileObject->PrivateCacheMap = NULL;							

							break;
						}
						else
						{
							RC = STATUS_OBJECT_PATH_NOT_FOUND;
							try_return( RC );
						}
					}

					if( NextRemainingName.Length )
					{
						//	Should be a directory...
						if( Type != EXT2_FT_DIR )
						{
							//	Invalid path...
							//	Can have only a directory in the middle of the path...
							//
							RC = STATUS_OBJECT_PATH_NOT_FOUND;
							try_return( RC );
						}

						PtrCurrFileObject = NULL;
					}
					else
					{
						//
						//	Done Parsing...
						//	Found the file...
						//
						Found = TRUE;
						
						//
						//	Was I supposed to create a new file?
						//
						if (RequestedDisposition == FILE_CREATE &&
							ReturnedInformation != FILE_CREATED ) 
						{
							ReturnedInformation = FILE_EXISTS;
							RC = STATUS_OBJECT_NAME_COLLISION;
							try_return(RC);
						}
						
						//	Is this the type of file I was looking for?
						//	Do some checking here...

						if( Type != EXT2_FT_DIR && Type != EXT2_FT_REG_FILE )
						{
							//	Deny access!
							//	Cannot open a special file...
							RC = STATUS_ACCESS_DENIED;
							try_return( RC );

						}
						if( DirectoryOnlyRequested && Type != EXT2_FT_DIR )
						{
							RC = STATUS_NOT_A_DIRECTORY;
							try_return( RC );
						}
						if( FileOnlyRequested && Type == EXT2_FT_DIR )
						{
							RC = STATUS_FILE_IS_A_DIRECTORY;
							try_return(RC);
						}

						PtrCurrFileObject = PtrNewFileObject;
						//	Things seem to be ok enough!
						//	Proceeing with the Open/Create...
						
					}

				
					//
					//	Create an FCB and initialise it...
					//
					{
						PtrExt2ObjectName		PtrObjectName;
						
						//	Initialising the object name...
						PtrObjectName = Ext2AllocateObjectName();
						Ext2CopyUnicodeString( &PtrObjectName->ObjectName, &CurrentName ); 
						//	RtlInitUnicodeString( &PtrObjectName->ObjectName, CurrentName.Buffer );

						if( !NT_SUCCESS( Ext2CreateNewFCB( 
							&PtrNextFCB,				//	the new FCB
							ZeroSize,					//	AllocationSize,
							ZeroSize,					//	EndOfFile,
             				PtrCurrFileObject,			//	The File Object
							PtrVCB,
							PtrObjectName  )  )  )
						{
							RC = STATUS_INSUFFICIENT_RESOURCES;
							try_return(RC);
						}

						if( Type == EXT2_FT_DIR )
							PtrNextFCB->FCBFlags |= EXT2_FCB_DIRECTORY;
						else if( Type != EXT2_FT_REG_FILE )
							PtrNextFCB->FCBFlags |= EXT2_FCB_SPECIAL_FILE;

						PtrNextFCB->INodeNo = CurrInodeNo ;
						PtrNextFCB->ParentINodeNo = PtrCurrFCB->INodeNo;

						if( PtrCurrFileObject == NULL && CurrInodeNo != EXT2_ROOT_INO )
						{
							//	This is an FCB created to cache the reads done while parsing
							//	Put this FCB on the ClosableFCBList 
							if( !PtrNextFCB->ClosableFCBs.OnClosableFCBList )
							{
									InsertTailList( &PtrVCB->ClosableFCBs.ClosableFCBListHead,
										&PtrNextFCB->ClosableFCBs.ClosableFCBList );
									PtrVCB->ClosableFCBs.Count++;
									PtrNextFCB->ClosableFCBs.OnClosableFCBList = TRUE;
							}
						}
					}
				}
	
				//
				//	Still not done parsing...
				//	miles to go before I open... ;)
				//
				PtrCurrFCB = PtrNextFCB;
			}

			PtrNewFCB = PtrNextFCB;
		}
		

		//	If I get this far...
		//	it means, I have located the file...
		//	I even have an FCB to represent it!!!

		if ( NT_SUCCESS (RC) ) 
		{

			if ((PtrNewFCB->FCBFlags & EXT2_FCB_DIRECTORY) && ((RequestedDisposition == FILE_SUPERSEDE) ||
				  (RequestedDisposition == FILE_OVERWRITE) || (RequestedDisposition == FILE_OVERWRITE_IF ))) 
			{
				RC = STATUS_FILE_IS_A_DIRECTORY;
				try_return(RC);
			}
		
			
			// Check share access and fail if the share conflicts with an existing
			// open.
			
			if (PtrNewFCB->OpenHandleCount > 0) 
			{
				// The FCB is currently in use by some thread.
				// We must check whether the requested access/share access
				// conflicts with the existing open operations.

				if (!NT_SUCCESS(RC = IoCheckShareAccess(DesiredAccess, ShareAccess, PtrNewFileObject,
												&(PtrNewFCB->FCBShareAccess), TRUE))) 
				{
					// Ext2CloseCCB(PtrNewCCB);
					try_return(RC);
				}
			} 
			else 
			{
				IoSetShareAccess(DesiredAccess, ShareAccess, PtrNewFileObject, &(PtrNewFCB->FCBShareAccess));
			}

			//
			//	Allocating a new CCB Structure...
			//	
			Ext2CreateNewCCB( &PtrNewCCB, PtrNewFCB, PtrNewFileObject);
			PtrNewFileObject->FsContext2 = (void *) PtrNewCCB;
			Ext2CopyUnicodeString( &(PtrNewCCB->AbsolutePathName), &AbsolutePathName );

			if( ReturnedInformation == -1 )
			{
				//	
				//	ReturnedInformation has not been set so far...
				//
				ReturnedInformation = FILE_OPENED;
			}

			// If a supersede or overwrite was requested, do so now ...
			if (RequestedDisposition == FILE_SUPERSEDE) 
			{
				// Attempt the operation here ...
				if( Ext2SupersedeFile( PtrNewFCB, PtrIrpContext) )
				{
					ReturnedInformation = FILE_SUPERSEDED;
				}
			}
			
			else if ((RequestedDisposition == FILE_OVERWRITE) || (RequestedDisposition == FILE_OVERWRITE_IF))
			{
				// Attempt the overwrite operation...
				if( Ext2OverwriteFile( PtrNewFCB, PtrIrpContext) )
				{
					ReturnedInformation = FILE_OVERWRITTEN;
				}
			}
			if( AllocationSize )
			{
				if( ReturnedInformation == FILE_CREATED ||
					ReturnedInformation == FILE_SUPERSEDED )
				{
					ULONG CurrentSize;
					ULONG LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
					
					for( CurrentSize = 0; CurrentSize < AllocationSize; CurrentSize += LogicalBlockSize )
					{
						Ext2AddBlockToFile( PtrIrpContext, PtrVCB, PtrNewFCB, PtrNewFileObject, FALSE );
					}
				}
			}

			if( ReturnedInformation == FILE_CREATED )
			{
				//	Allocate some data blocks if 
				//		1. initial file size has been specified...
				//		2. if the file is a Directory...
				//			In case of (2) make entries for '.' and '..'
				//	Zero out the Blocks...

				UNICODE_STRING		Name;

				if( DirectoryOnlyRequested )
				{

					Ext2CopyCharToUnicodeString( &Name, ".", 1 );
					Ext2MakeNewDirectoryEntry( PtrIrpContext, PtrNewFCB, PtrNewFileObject, &Name, EXT2_FT_DIR, PtrNewFCB->INodeNo);

					Name.Buffer[1] = '.';
					Name.Buffer[2] = '\0';
					Name.Length += 2;
					Ext2MakeNewDirectoryEntry( PtrIrpContext, PtrNewFCB, PtrNewFileObject, &Name, EXT2_FT_DIR, PtrNewFCB->ParentINodeNo );
					Ext2DeallocateUnicodeString( &Name );
				}
			}
			if( OpenTargetDirectory )
			{
				//
				//	Save the taget file name in the CCB...
				//
				Ext2CopyUnicodeString( 
					&PtrNewCCB->RenameLinkTargetFileName,
					&RenameLinkTargetFileName );
				Ext2DeallocateUnicodeString( &RenameLinkTargetFileName );
			}
		}

		try_exit:	NOTHING;

	}
	finally 
	{
		if (AcquiredVCB) 
		{
			ASSERT(PtrVCB);
			Ext2ReleaseResource(&(PtrVCB->VCBResource));

			AcquiredVCB = FALSE;
			DebugTrace(DEBUG_TRACE_MISC,   "*** VCB released [Create]", 0);

			if( PtrNewFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Create]", PtrNewFileObject);
			}
		}

		if (AbsolutePathName.Buffer != NULL) 
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Create]", AbsolutePathName.Buffer );
			ExFreePool(AbsolutePathName.Buffer);
		}

		// Complete the request unless we are here as part of unwinding
		//	when an exception condition was encountered, OR
		//	if the request has been deferred (i.e. posted for later handling)
		if (RC != STATUS_PENDING) 
		{
			// If we acquired any FCB resources, release them now ...

			// If any intermediate (directory) open operations were performed,
			//	implement the corresponding close (do *not* however close
			//	the target you have opened on behalf of the caller ...).

			if (NT_SUCCESS(RC)) 
			{
				// Update the file object such that:
				//	(a) the FsContext field points to the NTRequiredFCB field
				//		 in the FCB
				//	(b) the FsContext2 field points to the CCB created as a
				//		 result of the open operation

				// If write-through was requested, then mark the file object
				//	appropriately
				if (WriteThroughRequested) 
				{
					PtrNewFileObject->Flags |= FO_WRITE_THROUGH;
				}
				DebugTrace( DEBUG_TRACE_SPECIAL,   " === Create/Open successful", 0 );
			} 
			else 
			{
				DebugTrace( DEBUG_TRACE_SPECIAL,   " === Create/Open failed", 0 );
				// Perform failure related post-processing now
			}

			// As long as this unwinding is not being performed as a result of
			//	an exception condition, complete the IRP ...
			if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_EXCEPTION)) 
			{
				PtrIrp->IoStatus.Status = RC;
				PtrIrp->IoStatus.Information = ReturnedInformation;
			
				// Free up the Irp Context
				Ext2ReleaseIrpContext(PtrIrpContext);
				
				// complete the IRP
				IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
			}
		}
	}
	return(RC);
}


/*************************************************************************
*
* Function: Ext2OpenVolume()
*
* Description:
*	Open a logical volume for the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2OpenVolume(
PtrExt2VCB				PtrVCB,					// volume to be opened
PtrExt2IrpContext		PtrIrpContext,			// IRP context
PIRP						PtrIrp,					// original/user IRP
unsigned short			ShareAccess,			// share access
PIO_SECURITY_CONTEXT	PtrSecurityContext,	// caller's context (incl access)
PFILE_OBJECT			PtrNewFileObject)		// I/O Mgr. created file object
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2CCB			PtrCCB = NULL;

	try {
		// check for exclusive open requests (using share modes supplied)
		//	and determine whether it is even possible to open the volume
		//	with the specified share modes (e.g. if caller does not
		//	wish to share read or share write ...)
	
		//	Use IoCheckShareAccess() and IoSetShareAccess() here ...	
		//	They are defined in the DDK.

		//	You might also wish to check the caller's security context
		//	to see whether you wish to allow the volume open or not.
		//	Use the SeAccessCheck() routine described in the DDK for	this purpose.
	
		// create a new CCB structure
		if (!(PtrCCB = Ext2AllocateCCB())) 
		{
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}

		// initialize the CCB
		PtrCCB->PtrFCB = (PtrExt2FCB)(PtrVCB);
		InsertTailList(&(PtrVCB->VolumeOpenListHead), &(PtrCCB->NextCCB));

		// initialize the CCB to point to the file object
		PtrCCB->PtrFileObject = PtrNewFileObject;

		Ext2SetFlag(PtrCCB->CCBFlags, EXT2_CCB_VOLUME_OPEN);

		// initialize the file object appropriately
		PtrNewFileObject->FsContext = (void *)( &(PtrVCB->CommonVCBHeader) );
		PtrNewFileObject->FsContext2 = (void *)(PtrCCB);

		// increment the number of outstanding open operations on this
		//	logical volume (i.e. volume cannot be dismounted)

		//	You might be concerned about 32 bit wrap-around though I would
		//	argue that it is unlikely ... :-)
		(PtrVCB->VCBOpenCount)++;
	
		// now set the IoStatus Information value correctly in the IRP
		//	(caller will set the status field)
		PtrIrp->IoStatus.Information = FILE_OPENED;
	
		try_exit:	NOTHING;
	} 
	finally 
	{
		NOTHING;
	}

	return(RC);
}

/*************************************************************************
*
* Function: Ext2InitializeFCB()
*
* Description:
*	Initialize a new FCB structure and also the sent-in file object
*	(if supplied)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************
void Ext2InitializeFCB(
PtrExt2FCB				PtrNewFCB,		// FCB structure to be initialized
PtrExt2VCB				PtrVCB,			// logical volume (VCB) pointer
PtrExt2ObjectName		PtrObjectName,	// name of the object
uint32					Flags,			// is this a file/directory, etc.
PFILE_OBJECT			PtrFileObject)	// optional file object to be initialized
{
	// Initialize the disk dependent portion as you see fit

	// Initialize the two ERESOURCE objects
	ExInitializeResourceLite(&(PtrNewFCB->NTRequiredFCB.MainResource));
	ExInitializeResourceLite(&(PtrNewFCB->NTRequiredFCB.PagingIoResource));

	PtrNewFCB->FCBFlags |= EXT2_INITIALIZED_MAIN_RESOURCE | EXT2_INITIALIZED_PAGING_IO_RESOURCE | Flags;

	PtrNewFCB->PtrVCB = PtrVCB;

	// caller MUST ensure that VCB has been acquired exclusively
	InsertTailList(&(PtrVCB->FCBListHead), &(PtrNewFCB->NextFCB));

	// initialize the various list heads
	InitializeListHead(&(PtrNewFCB->CCBListHead));

//	PtrNewFCB->ReferenceCount = 1;
//	PtrNewFCB->OpenHandleCount = 1;

	if( PtrObjectName )
	{
		PtrNewFCB->FCBName = PtrObjectName;
	}

	if ( PtrFileObject )
	{
		PtrFileObject->FsContext = (void *)(&(PtrNewFCB->NTRequiredFCB));
	}

	return;
}
*/

/*************************************************************************
*
* Function: Ext2OpenRootDirectory()
*
* Description:
*	Open the root directory for a volume
*	
*
* Expected Interrupt Level (for execution) :
*
*  ???
*
* Return Value: None
*
*************************************************************************/
NTSTATUS NTAPI Ext2OpenRootDirectory(
	PtrExt2VCB				PtrVCB,					// volume 
	PtrExt2IrpContext		PtrIrpContext,			// IRP context
	PIRP					PtrIrp,					// original/user IRP
	unsigned short			ShareAccess,			// share access
	PIO_SECURITY_CONTEXT	PtrSecurityContext,		// caller's context (incl access)
	PFILE_OBJECT			PtrNewFileObject		// I/O Mgr. created file object
	)
{
	//	Declerations...
	PtrExt2CCB PtrCCB;
	
	ASSERT( PtrVCB );
	ASSERT( PtrVCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);
	ASSERT( PtrVCB->PtrRootDirectoryFCB );
	AssertFCB( PtrVCB->PtrRootDirectoryFCB );
		
	PtrVCB->PtrRootDirectoryFCB->INodeNo = EXT2_ROOT_INO;
	
	//	Create a new CCB...
	Ext2CreateNewCCB( &PtrCCB, PtrVCB->PtrRootDirectoryFCB, PtrNewFileObject);
	PtrNewFileObject->FsContext = (void *) &(PtrVCB->PtrRootDirectoryFCB->NTRequiredFCB.CommonFCBHeader);
	PtrVCB->PtrRootDirectoryFCB->FCBFlags |= EXT2_FCB_DIRECTORY;
	PtrNewFileObject->FsContext2 = (void *) PtrCCB;
	PtrNewFileObject->SectionObjectPointer = &PtrVCB->PtrRootDirectoryFCB->NTRequiredFCB.SectionObject;
	PtrNewFileObject->Vpb = PtrVCB->PtrVPB;

	Ext2CopyUnicodeString( &(PtrCCB->AbsolutePathName), &PtrVCB->PtrRootDirectoryFCB->FCBName->ObjectName );


	return STATUS_SUCCESS;
}



PtrExt2FCB NTAPI Ext2LocateChildFCBInCore(
	PtrExt2VCB				PtrVCB,	
	PUNICODE_STRING			PtrName, 
	ULONG					ParentInodeNo )
{

	PtrExt2FCB PtrFCB = NULL;
	PLIST_ENTRY	PtrEntry;

	if( IsListEmpty( &(PtrVCB->FCBListHead) ) )
	{
		return NULL;	//	Failure;
	}

	for( PtrEntry = PtrVCB->FCBListHead.Flink; 
			PtrEntry != &PtrVCB->FCBListHead; 
			PtrEntry = PtrEntry->Flink )
	{
		PtrFCB = CONTAINING_RECORD( PtrEntry, Ext2FCB, NextFCB );
		ASSERT( PtrFCB );
		if( PtrFCB->ParentINodeNo == ParentInodeNo )
		{
			if( RtlCompareUnicodeString( &PtrFCB->FCBName->ObjectName, PtrName, TRUE ) == 0 )
				return PtrFCB;
		}
	}

	return NULL;
}

PtrExt2FCB NTAPI Ext2LocateFCBInCore(
	PtrExt2VCB				PtrVCB,	
	ULONG					InodeNo )
{
	PtrExt2FCB PtrFCB = NULL;
	PLIST_ENTRY	PtrEntry;

	if( IsListEmpty( &(PtrVCB->FCBListHead) ) )
	{
		return NULL;	//	Failure;
	}

	for( PtrEntry = PtrVCB->FCBListHead.Flink; 
			PtrEntry != &PtrVCB->FCBListHead; 
			PtrEntry = PtrEntry->Flink )
	{
		PtrFCB = CONTAINING_RECORD( PtrEntry, Ext2FCB, NextFCB );
		ASSERT( PtrFCB );
		if( PtrFCB->INodeNo == InodeNo )
		{
			return PtrFCB;
		}
	}

	return NULL;
}


ULONG NTAPI Ext2LocateFileInDisk (
	PtrExt2VCB				PtrVCB,
	PUNICODE_STRING			PtrCurrentName, 
	PtrExt2FCB				PtrParentFCB,
	ULONG					*Type )
{

	PFILE_OBJECT		PtrFileObject = NULL;
	ULONG				InodeNo = 0;
	
	*Type = EXT2_FT_UNKNOWN;

	//	1. 
	//	Initialize the Blocks in the FCB...
	//
	Ext2InitializeFCBInodeInfo( PtrParentFCB );

	
	//	2.
	//	Is there a file object I can use for caching??
	//	If not create one...
	//
	if( !PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject )
	{
		//
		//	No Directory File Object?
		//	No problem, will create one...
		//

		//	Acquire the MainResource first though...
			
			PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject = IoCreateStreamFileObject(NULL, PtrVCB->TargetDeviceObject );
			PtrFileObject = PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject;
			
			if( !PtrFileObject )
			{
				Ext2BreakPoint();
				return 0;
			}
			PtrFileObject->ReadAccess = TRUE;
			PtrFileObject->WriteAccess = TRUE;

			//	Associate the file stream with the Volume parameter block...
			PtrFileObject->Vpb = PtrVCB->PtrVPB;

			//	No caching as yet...
			PtrFileObject->PrivateCacheMap = NULL;

			//	this establishes the FCB - File Object connection...
			PtrFileObject->FsContext = (void *)( & (PtrParentFCB->NTRequiredFCB.CommonFCBHeader) );

			//	Initialize the section object pointer...
			PtrFileObject->SectionObjectPointer = &(PtrParentFCB->NTRequiredFCB.SectionObject);
	}
	else
	{
		//	
		//	I do have a file object... 
		//	I am using it now!
		//
		PtrFileObject = PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject;
	}

	//	3.
	//	Got hold of a file object? Good	;)
	//	Now initiating Caching, pinned access to be precise ...
	//
	if (PtrFileObject->PrivateCacheMap == NULL) 
	{
			CcInitializeCacheMap(PtrFileObject, (PCC_FILE_SIZES)(&(PtrParentFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize)),
				TRUE,		// We utilize pin access for directories
				&(Ext2GlobalData.CacheMgrCallBacks), // callbacks
				PtrParentFCB );		// The context used in callbacks
	}

	//	4.
	//	Getting down to the real business now... ;)
	//	Read in the directory contents and do a search 
	//	a sequential search to be precise...
	//	Wish Mm'm Renuga were reading this
	//	Would feel proud...	;)
	//
	{
		LARGE_INTEGER	StartBufferOffset;
		ULONG			PinBufferLength;
		ULONG			BufferIndex;
		PBCB			PtrBCB = NULL;
		BYTE *			PtrPinnedBlockBuffer = NULL;
		PEXT2_DIR_ENTRY	PtrDirEntry = NULL;
		BOOLEAN			Found;
		int				i;


		StartBufferOffset.QuadPart = 0;

		//
		//	Read in the whole damn directory
		//	**Bad programming**
		//	Will do for now.
		//
		PinBufferLength = PtrParentFCB->NTRequiredFCB.CommonFCBHeader.FileSize.LowPart;
		if (!CcMapData( PtrFileObject,
                  &StartBufferOffset,
                  PinBufferLength,
                  TRUE,
                  &PtrBCB,
                  (PVOID*)&PtrPinnedBlockBuffer ) )
		{

			
		}
		//
		//	Walking through now...
		//
		
		for( BufferIndex = 0, Found = FALSE; !Found && BufferIndex < ( PtrParentFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) ; BufferIndex += PtrDirEntry->rec_len )
		{
			PtrDirEntry = (PEXT2_DIR_ENTRY) &PtrPinnedBlockBuffer[ BufferIndex ];
			if( PtrDirEntry->name_len == 0 || PtrDirEntry->rec_len == 0 || PtrDirEntry->inode == 0)
			{
				//	Invalid entry...
				//  Ignore...
				continue;
			}
			//
			//	Comparing ( case sensitive )
			//	Directory entry is not NULL terminated...
			//	nor is the CurrentName...
			//
			if( PtrDirEntry->name_len != (PtrCurrentName->Length / 2) )
				continue;
			
			for( i = 0, Found = TRUE ; i < PtrDirEntry->name_len ; i++ )
			{
				if( PtrDirEntry->name[ i ] != PtrCurrentName->Buffer[ i ] )
				{
					Found = FALSE;
					break;

				}
			}
			
		}
		if( Found )
		{
			InodeNo = PtrDirEntry->inode;

			if( PtrDirEntry->file_type == EXT2_FT_UNKNOWN )
			{

				//	Old Fashioned Directory entries...
				//	Will have to read in the Inode to determine the File Type...
				EXT2_INODE	Inode;
				//	PtrInode = Ext2AllocatePool( NonPagedPool, sizeof( EXT2_INODE )  );
				Ext2ReadInode( PtrVCB, InodeNo, &Inode );	

				if( Ext2IsModeRegularFile( Inode.i_mode ) )
				{
					*Type = EXT2_FT_REG_FILE;
				}
				else if ( Ext2IsModeDirectory( Inode.i_mode) )
				{
					//	Directory...
					*Type = EXT2_FT_DIR;
				}
				else if( Ext2IsModeSymbolicLink(Inode.i_mode) )
				{
					*Type = EXT2_FT_SYMLINK;
				}
				else if( Ext2IsModePipe(Inode.i_mode) )
				{
					*Type = EXT2_FT_FIFO;
				}
				else if( Ext2IsModeCharacterDevice(Inode.i_mode) )
				{
					*Type = EXT2_FT_CHRDEV;
				}
				else if( Ext2IsModeBlockDevice(Inode.i_mode) )
				{
					*Type = EXT2_FT_BLKDEV;
				}
				else if( Ext2IsModeSocket(Inode.i_mode) )
				{
					*Type = EXT2_FT_SOCK;
				}
				else
				{
					*Type = EXT2_FT_UNKNOWN;
				}
				
				//DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Create]", PtrInode );
				//ExFreePool( PtrInode );
			}
			else
			{
				*Type = PtrDirEntry->file_type;
			}
		}

		CcUnpinData( PtrBCB );
		PtrBCB = NULL;

		return InodeNo;
	}
}


/*************************************************************************
*
* Function: Ext2CreateFile()
*
* Description:
*	Creates a new file on the disk
*	
* Expected Interrupt Level (for execution) :
*	IRQL_PASSIVE_LEVEL
*
* Restrictions:
*	Expects the VCB to be acquired Exclusively before being invoked
*
* Return Value: None
*
*************************************************************************/
ULONG NTAPI Ext2CreateFile( 
	PtrExt2IrpContext		PtrIrpContext,
	PtrExt2VCB				PtrVCB,
	PUNICODE_STRING			PtrName, 
	PtrExt2FCB				PtrParentFCB,
	ULONG					Type)
{
	EXT2_INODE	Inode, ParentInode;

	ULONG		NewInodeNo = 0;
	BOOLEAN		FCBAcquired = FALSE;

	ULONG		LogicalBlockSize = 0;

	try
	{
		

		//	0. Verify if the creation is possible,,,
		if( Type != EXT2_FT_DIR && Type != EXT2_FT_REG_FILE )
		{
			//
			//	Can create only a directory or a regular file...
			//
			return 0;
		}
		
		//	1. Allocate an i-node...
		
		NewInodeNo = Ext2AllocInode( PtrIrpContext, PtrVCB, PtrParentFCB->INodeNo );
		
		//	NewInodeNo = 12;

		if( !NewInodeNo )
		{
			return 0;
		}
		
		//	2. Acquire the Parent FCB Exclusively...
		if( !ExAcquireResourceExclusiveLite(&( PtrParentFCB->NTRequiredFCB.MainResource ), TRUE) )
		{
			Ext2DeallocInode( PtrIrpContext, PtrVCB, NewInodeNo );
			try_return( NewInodeNo = 0);
		}
		FCBAcquired = TRUE;

		//	3. Make an entry in the parent Directory...		
		ASSERT( PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject );
		
		Ext2MakeNewDirectoryEntry(
			PtrIrpContext,
			PtrParentFCB, 
			PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject, 
			PtrName, Type, NewInodeNo );
	
		
		//	4. Initialize an inode entry and  write it to disk...
		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

		{
			//	To be deleted
			Ext2ReadInode( PtrVCB, NewInodeNo, &Inode );
		}

		RtlZeroMemory( &Inode, sizeof( EXT2_INODE ) );
		if( Type == EXT2_FT_DIR )
		{
			Inode.i_mode = 0x41ff;

			//	In addition to the usual link, 
			//	there will be an additional link in the directory itself - the '.' entry
			Inode.i_links_count = 2;

			//	Incrementing the link count for the parent as well...
			Ext2ReadInode( PtrVCB, PtrParentFCB->INodeNo, &ParentInode );
			ParentInode.i_links_count++;
			Ext2WriteInode( PtrIrpContext, PtrVCB, PtrParentFCB->INodeNo, &ParentInode );

		}
		else
		{
			Inode.i_mode = 0x81ff;
			Inode.i_links_count = 1;
		}


		{
			//
			//	Setting the time fields in the inode...
			//
			ULONG Time;
			Time = Ext2GetCurrentTime();
			Inode.i_ctime = Time;
			Inode.i_atime = Time;
			Inode.i_mtime = Time;
			Inode.i_dtime = 0;			//	Deleted time;
		}

		Ext2WriteInode( PtrIrpContext, PtrVCB, NewInodeNo, &Inode );

		try_exit:	NOTHING;
	}
	finally 
	{
		if( FCBAcquired )
		{
			Ext2ReleaseResource( &(PtrParentFCB->NTRequiredFCB.MainResource) );
		}
	}

	return NewInodeNo ;
}

/*************************************************************************
*
* Function: Ext2CreateFile()
*
* Description:
*	Overwrites an existing file on the disk
*	
* Expected Interrupt Level (for execution) :
*	IRQL_PASSIVE_LEVEL
*
* Restrictions:
*	Expects the VCB to be acquired Exclusively before being invoked
*
* Return Value: None
*
*************************************************************************/
BOOLEAN NTAPI Ext2OverwriteFile(
	PtrExt2FCB			PtrFCB,
	PtrExt2IrpContext	PtrIrpContext)
{
	EXT2_INODE			Inode;
	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;
	ULONG				i;
	ULONG	Time;
	Time = Ext2GetCurrentTime();

	Ext2InitializeFCBInodeInfo( PtrFCB );
	//	1.
	//	Update the inode...
	if( !NT_SUCCESS( Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		return FALSE;
	}

	Inode.i_size = 0;
	Inode.i_blocks = 0;
	Inode.i_atime = Time;
	Inode.i_mtime = Time;
	Inode.i_dtime = 0;

	for( i = 0; i < EXT2_N_BLOCKS; i++ )
	{
		Inode.i_block[ i ] = 0;
	}

	if( !NT_SUCCESS( Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		return FALSE;
	}

	//	2.
	//	Release all the data blocks...
	if( !Ext2ReleaseDataBlocks( PtrFCB, PtrIrpContext) )
	{
		return FALSE;
	}
	
	Ext2ClearFlag( PtrFCB->FCBFlags, EXT2_FCB_BLOCKS_INITIALIZED );
	Ext2InitializeFCBInodeInfo( PtrFCB );
	
	return TRUE;
}

/*************************************************************************
*
* Function: Ext2SupersedeFile()
*
* Description:
*	Supersedes an existing file on the disk
*	
* Expected Interrupt Level (for execution) :
*	IRQL_PASSIVE_LEVEL
*
* Restrictions:
*	Expects the VCB to be acquired Exclusively before being invoked
*
* Return Value: None
*
*************************************************************************/
BOOLEAN NTAPI Ext2SupersedeFile(
	PtrExt2FCB			PtrFCB,
	PtrExt2IrpContext	PtrIrpContext)
{
	EXT2_INODE			Inode;
	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;

	Ext2InitializeFCBInodeInfo( PtrFCB );

	//	1.
	//	Initialize the inode...
	RtlZeroMemory( &Inode, sizeof( EXT2_INODE ) );

	//	Setting the file mode...
	//	This operation is allowed only for a regular file...
	Inode.i_mode = 0x81ff;

	//	Maintaining the old link count...
	Inode.i_links_count = PtrFCB->LinkCount;

	//	Setting the time fields in the inode...
	{
		ULONG Time;
		Time = Ext2GetCurrentTime();
		Inode.i_ctime = Time;
		Inode.i_atime = Time;
		Inode.i_mtime = Time;
		Inode.i_dtime = 0;			//	Deleted time;
	}

	if( !NT_SUCCESS( Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		return FALSE;
	}

	//	2.
	//	Release all the data blocks...
	if( !Ext2ReleaseDataBlocks( PtrFCB, PtrIrpContext) )
	{
		return FALSE;
	}
	
	Ext2ClearFlag( PtrFCB->FCBFlags, EXT2_FCB_BLOCKS_INITIALIZED );
	Ext2InitializeFCBInodeInfo( PtrFCB );
	
	return TRUE;
}

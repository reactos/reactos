/*************************************************************************
*
* File: dircntrl.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "directory control" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_DIR_CONTROL

#define			DEBUG_LEVEL						(DEBUG_TRACE_DIRCTRL)


/*************************************************************************
*
* Function: Ext2DirControl()
*
* Description:
*	The I/O Manager will invoke this routine to handle a directory control
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
NTSTATUS NTAPI Ext2DirControl(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS			RC = STATUS_SUCCESS;
    PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

 	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "DIR Control IRP received...", 0);

	
	FsRtlEnterFileSystem();

	ASSERT(DeviceObject);
	ASSERT(Irp);

	// set the top level context
	AreWeTopLevel = Ext2IsIrpTopLevel(Irp);

	try 
	{
		// get an IRP context structure and issue the request
		PtrIrpContext = Ext2AllocateIrpContext(Irp, DeviceObject);
		ASSERT(PtrIrpContext);

		RC = Ext2CommonDirControl(PtrIrpContext, Irp);

	}
	except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
	{

		RC = Ext2ExceptionHandler(PtrIrpContext, Irp);

		Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
	}

	if (AreWeTopLevel) {
		IoSetTopLevelIrp(NULL);
	}

	FsRtlExitFileSystem();

	return(RC);
}



/*************************************************************************
*
* Function: Ext2CommonDirControl()
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
NTSTATUS NTAPI Ext2CommonDirControl(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp)
{
	NTSTATUS					RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION	PtrIoStackLocation = NULL;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;

	// First, get a pointer to the current I/O stack location
	PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
	ASSERT(PtrIoStackLocation);

	PtrFileObject = PtrIoStackLocation->FileObject;
	ASSERT(PtrFileObject);

	// Get the FCB and CCB pointers
	PtrCCB = (PtrExt2CCB)(PtrFileObject->FsContext2);
	ASSERT(PtrCCB);
	PtrFCB = PtrCCB->PtrFCB;

	AssertFCB( PtrFCB );
	

	// Get some of the parameters supplied to us
	switch (PtrIoStackLocation->MinorFunction) {
	case IRP_MN_QUERY_DIRECTORY:
#ifdef _GNU_NTIFS_
		RC = Ext2QueryDirectory(PtrIrpContext, PtrIrp, (PEXTENDED_IO_STACK_LOCATION)PtrIoStackLocation, PtrFileObject, PtrFCB, PtrCCB);
#else
		RC = Ext2QueryDirectory(PtrIrpContext, PtrIrp, PtrIoStackLocation, PtrFileObject, PtrFCB, PtrCCB);
#endif
		break;
	case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
		{
			RC = STATUS_NOT_IMPLEMENTED;
			PtrIrp->IoStatus.Status = RC;
			PtrIrp->IoStatus.Information = 0;
			IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
		}
		//	RC = Ext2NotifyChangeDirectory(PtrIrpContext, PtrIrp, PtrIoStackLocation, PtrFileObject, PtrFCB, PtrCCB);
		break;
	default:
		// This should not happen.
		RC = STATUS_INVALID_DEVICE_REQUEST;
		PtrIrp->IoStatus.Status = RC;
		PtrIrp->IoStatus.Information = 0;

		// Free up the Irp Context
		Ext2ReleaseIrpContext(PtrIrpContext);

		// complete the IRP
		IoCompleteRequest(PtrIrp, IO_NO_INCREMENT);
		break;
	}

	return(RC);
}


/*************************************************************************
*
* Function: Ext2QueryDirectory()
*
* Description:
*	Query directory request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2QueryDirectory(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
#ifdef _GNU_NTIFS_
PEXTENDED_IO_STACK_LOCATION			PtrIoStackLocation,
#else
PIO_STACK_LOCATION			PtrIoStackLocation,
#endif
PFILE_OBJECT				PtrFileObject,
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	BOOLEAN					PostRequest = FALSE;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	BOOLEAN					CanWait = FALSE;
	PtrExt2VCB				PtrVCB = NULL;
	BOOLEAN					AcquiredFCB = FALSE;
	unsigned long			BufferLength = 0;
	unsigned long			BufferIndex	= 0;
	unsigned long			FileIndex = 0;
	PUNICODE_STRING			PtrSearchPattern = NULL;
	FILE_INFORMATION_CLASS	FileInformationClass;
	BOOLEAN					RestartScan = FALSE;
	BOOLEAN					ReturnSingleEntry = FALSE;
	BOOLEAN					IndexSpecified = FALSE;
	unsigned char			*Buffer = NULL;
	BOOLEAN					FirstTimeQuery = FALSE;
	unsigned long			StartingIndexForSearch = 0;
	unsigned long			BytesReturned = 0;
	BOOLEAN					BufferUsedup = FALSE;

	BOOLEAN					SearchWithWildCards = FALSE;
	
	PFILE_BOTH_DIR_INFORMATION		BothDirInformation = NULL;
	PFILE_DIRECTORY_INFORMATION		DirectoryInformation = NULL;

	
	PEXT2_DIR_ENTRY		PtrDirEntry = NULL;
	PEXT2_INODE			PtrInode	= NULL;
	
	unsigned long		LogicalBlockSize;
	
	unsigned long		ThisBlock;
	
	//	The starting Physical Block No...
	//LARGE_INTEGER StartPhysicalBlock;
	LARGE_INTEGER StartBufferOffset ;
	ULONG PinBufferLength;
		
	//	Buffer Control Block
	PBCB				PtrBCB = NULL;
	BYTE *				PtrPinnedBlockBuffer = NULL;

	unsigned int j;
	
	DebugTrace(DEBUG_TRACE_MISC,   " === Querying Directory %S", PtrFCB->FCBName->ObjectName.Buffer );

	try 
	{
		// Validate the sent-in FCB
		if ((PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) || !(PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY)) 
		{
			// We will only allow notify requests on directories.
			RC = STATUS_INVALID_PARAMETER;
		}

		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);
		CanWait = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);
		PtrVCB = PtrFCB->PtrVCB;

		//
		//	Asynchronous IO requested
		//	Posting request...
		//
		/*
		 * This is incorrect because posted IRP_MJ_DIRECTORY_CONTROL
		 * requests aren't handled in the worker thread yet. I tried
		 * adding handling of them to the worked routine, but there
		 * were problems with accessing the PtrIoStackLocation->
		 * Parameters.QueryDirectory.FileName variable.
		 * -- Filip Navara, 18/08/2004
		 */
#if 0
		if (!CanWait) 
		{
			PostRequest = TRUE;
			try_return(RC = STATUS_PENDING);
		}
#endif

		// Obtain the callers parameters
		BufferLength = PtrIoStackLocation->Parameters.QueryDirectory.Length;
		PtrSearchPattern = ( PUNICODE_STRING	) PtrIoStackLocation->Parameters.QueryDirectory.FileName;
		FileInformationClass = PtrIoStackLocation->Parameters.QueryDirectory.FileInformationClass;
		FileIndex = PtrIoStackLocation->Parameters.QueryDirectory.FileIndex;

		// Some additional arguments that affect the FSD behavior
		RestartScan       = (PtrIoStackLocation->Flags & SL_RESTART_SCAN);
		ReturnSingleEntry = (PtrIoStackLocation->Flags & SL_RETURN_SINGLE_ENTRY);
		IndexSpecified    = (PtrIoStackLocation->Flags & SL_INDEX_SPECIFIED);

		//
		// Acquiring exclusive access to the FCB.
		// This is not mandatory
		//
		DebugTrace(DEBUG_TRACE_MISC,   "*** Going into a block to acquire FCB Exclusively[DirCtrl]", 0);

		DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [DirCtrl]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
		ExAcquireResourceExclusiveLite(&(PtrReqdFCB->MainResource), TRUE);
		
		DebugTrace(DEBUG_TRACE_MISC,   "*** FCB acquired [DirCtrl]", 0);
		AcquiredFCB = TRUE;

		// We must determine the buffer pointer to be used. Since this
		// routine could either be invoked directly in the context of the
		// calling thread, or in the context of a worker thread, here is
		// a general way of determining what we should use.
		Buffer = Ext2GetCallersBuffer ( PtrIrp );

		// The method of determining where to look from and what to look for is
		// unfortunately extremely confusing. However, here is a methodology you
		// you can broadly adopt:
		// (a) You have to maintain a search buffer per CCB structure.
		// (b) This search buffer is initialized the very first time
		//		 a query directory operation is performed using the file object.
		// (For the sample FSD, the search buffer is stored in the
		//	 DirectorySearchPattern field)
		// However, the caller still has the option of "overriding" this stored
		// search pattern by supplying a new one in a query directory operation.
		//

		if( PtrCCB->DirectorySearchPattern.Length )
		{
			if( PtrCCB->DirectorySearchPattern.Buffer[PtrCCB->DirectorySearchPattern.Length/2] != 0 )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "&&&&&&&&&  PtrCCB->DirectorySearchPattern not NULL terminated!", 0);
			}
			DebugTrace(DEBUG_TRACE_MISC,   " === Old Search pattern %S", PtrCCB->DirectorySearchPattern.Buffer );
		}

		if (PtrSearchPattern != NULL) 
		{
			// User has supplied a search pattern
			// Now validate that the search pattern is legitimate

			if ( PtrCCB->DirectorySearchPattern.Length == 0 ) 
			{
				// This must be the very first query request.
				FirstTimeQuery = TRUE;
			}
			else
			{
				// We should ignore the search pattern in the CCB and instead,
				// use the user-supplied pattern for this particular query
				// directory request.
				Ext2DeallocateUnicodeString( &PtrCCB->DirectorySearchPattern );
			}

			// Now, allocate enough memory to contain the caller
			// supplied search pattern and fill in the DirectorySearchPattern
			// field in the CCB
			Ext2CopyUnicodeString( &PtrCCB->DirectorySearchPattern, PtrSearchPattern );
			/*
			PtrCCB->DirectorySearchPattern = Ext2AllocatePool(PagedPool, sizeof( PtrSearchPattern ) );
			ASSERT(PtrCCB->DirectorySearchPattern);
			RtlCopyMemory( PtrCCB->DirectorySearchPattern, PtrSearchPattern, sizeof( PtrSearchPattern ) );
			*/
		}
		else if ( PtrCCB->DirectorySearchPattern.Length == 0 ) 
		{
			// This MUST be the first directory query operation (else the
			// DirectorySearchPattern field would never be empty. Also, the caller
			// has neglected to provide a pattern so we MUST invent one.
			// Use "*" (following NT conventions) as your search pattern
			// and store it in the PtrCCB->DirectorySearchPattern field.
			
			/*
			PtrCCB->DirectorySearchPattern = Ext2AllocatePool(PagedPool, sizeof(L"*") );
			ASSERT(PtrCCB->DirectorySearchPattern);
			RtlCopyMemory( PtrCCB->DirectorySearchPattern, L"*", 4 );*/

			Ext2CopyWideCharToUnicodeString( &PtrCCB->DirectorySearchPattern, L"*" );
			
			FirstTimeQuery = TRUE;
		} 
		else 
		{
			// The caller has not supplied any search pattern...
			// Using previously supplied pattern
			PtrSearchPattern = &PtrCCB->DirectorySearchPattern;
		}

		if( PtrCCB->DirectorySearchPattern.Buffer[PtrCCB->DirectorySearchPattern.Length/2] != 0 )
		{
			DebugTrace(DEBUG_TRACE_MISC,  "&&&&&&&&&  PtrCCB->DirectorySearchPattern not NULL terminated!", 0 );
		}
		DebugTrace(DEBUG_TRACE_MISC,   " === Search pattern %S", PtrCCB->DirectorySearchPattern.Buffer );
		SearchWithWildCards = FsRtlDoesNameContainWildCards( PtrSearchPattern );

		// There is one other piece of information that your FSD must store
		// in the CCB structure for query directory support. This is the index
		// value (i.e. the offset in your on-disk directory structure) from
		// which you should start searching.
		// However, the flags supplied with the IRP can make us override this
		// as well.

		if (FileIndex) 
		{
			// Caller has told us wherefrom to begin.
			// You may need to round this to an appropriate directory entry
			// entry alignment value.
			StartingIndexForSearch = FileIndex;
		} 
		else if (RestartScan) 
		{
			StartingIndexForSearch = 0;
		} 
		else 
		{
			// Get the starting offset from the CCB.
			StartingIndexForSearch = PtrCCB->CurrentByteOffset.LowPart;
		}

		//	Read in the file inode if it hasn't already been read...
		Ext2InitializeFCBInodeInfo( PtrFCB );
		
		if (PtrFileObject->PrivateCacheMap == NULL) 
		{
			CcInitializeCacheMap(PtrFileObject, (PCC_FILE_SIZES)(&(PtrReqdFCB->CommonFCBHeader.AllocationSize)),
				TRUE,		// We will utilize pin access for directories
				&(Ext2GlobalData.CacheMgrCallBacks), // callbacks
				PtrCCB);		// The context used in callbacks
		}


		//
		//	Read in the next Data Block of this directory
		//
		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
		StartBufferOffset.QuadPart = ( StartingIndexForSearch / LogicalBlockSize );
		StartBufferOffset.QuadPart *= LogicalBlockSize;	//	This should be the StartBufferOffset alaigned to LBlock boundary...
		
		PinBufferLength = PtrReqdFCB->CommonFCBHeader.FileSize.LowPart - StartBufferOffset.LowPart;

		if ( !CcMapData( PtrFileObject,
                  &StartBufferOffset,
                  PinBufferLength,
                  TRUE,
                  &PtrBCB,
                  (PVOID*)&PtrPinnedBlockBuffer ) )
		{
			//	Read Failure
			DebugTrace(DEBUG_TRACE_MISC,   "Cache read failiure while reading in volume meta data", 0);
			try_return( STATUS_ACCESS_DENIED );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_MISC,   "Cache hit while reading in volume meta data", 0);
		}
		
		PtrInode = Ext2AllocatePool( PagedPool, sizeof( EXT2_INODE )  );

		//
		//	Walking through the directory entries...
		for( BufferUsedup = FALSE, BufferIndex = 0; !BufferUsedup && StartingIndexForSearch < ( PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) ; )
		{
			PtrDirEntry = (PEXT2_DIR_ENTRY) &PtrPinnedBlockBuffer[ StartingIndexForSearch - StartBufferOffset.LowPart ];
			
			StartingIndexForSearch += PtrDirEntry->rec_len;
			PtrCCB->CurrentByteOffset.LowPart = StartingIndexForSearch;

			if( PtrDirEntry->inode == 0 )
			{
				continue;
			}
			if( PtrDirEntry->name_len == 0 || PtrDirEntry->rec_len == 0 )
			{
				//
				//	This should not happen
				//	Hqw can this be so!!!
				//
				Ext2BreakPoint();
				if( BothDirInformation )
				{
					BothDirInformation->NextEntryOffset = 0;
				}
				if( !BytesReturned )
				{
					if( FirstTimeQuery )
						RC = STATUS_NO_SUCH_FILE;
					else
						RC = STATUS_NO_MORE_FILES;
				}
				break;
			}

			//	Does this entry match the search criterian?
			//	Checking
			//
			{
				UNICODE_STRING	FileName;
				LONG	Matched = 0;
				// Constructing a counted Unicode string out of PtrDirEntry
				Ext2CopyCharToUnicodeString( &FileName, PtrDirEntry->name, PtrDirEntry->name_len );

				if ( SearchWithWildCards )
				{
					Matched = FsRtlIsNameInExpression ( PtrSearchPattern, &FileName, FALSE, NULL );
				}
				else
				{
					Matched = ! RtlCompareUnicodeString( PtrSearchPattern, &FileName, FALSE );
				}
			
				Ext2DeallocateUnicodeString( &FileName );
				if( !Matched )
				{
					continue;
				}
			}

			switch( FileInformationClass )
			{
			case FileBothDirectoryInformation:
				
				DebugTrace(DEBUG_TRACE_DIRINFO,   " === FileBothDirectoryInformation", 0 );
				ThisBlock = sizeof( FILE_BOTH_DIR_INFORMATION );
				ThisBlock += PtrDirEntry->name_len*2;
				ThisBlock = Ext2QuadAlign( ThisBlock  );

				if( ( BufferIndex + ThisBlock ) > BufferLength )
				{
					//
					//	Next entry won't fit into the buffer...
					//	will have to return... 
					//	:(
					//
					if( BothDirInformation )
						BothDirInformation->NextEntryOffset = 0;
					if( !BytesReturned )
						RC = STATUS_NO_MORE_FILES;
					BufferUsedup = TRUE;
					break;
				}
							
				Ext2ReadInode( PtrVCB, PtrDirEntry->inode, PtrInode );
				if( !PtrInode )
				{
					try_return( RC = STATUS_UNSUCCESSFUL );
				}

				BothDirInformation = ( PFILE_BOTH_DIR_INFORMATION ) ( Buffer + ( BufferIndex ) );
				BothDirInformation->EaSize					= 0;
				BothDirInformation->AllocationSize.QuadPart	= PtrInode->i_blocks * 512;
				BothDirInformation->EndOfFile.QuadPart		= PtrInode->i_size;
				BothDirInformation->ChangeTime.QuadPart		= 0;

				BothDirInformation->CreationTime.QuadPart	= ( __int64 ) PtrInode->i_ctime * 10000000;
				BothDirInformation->CreationTime.QuadPart	+= Ext2GlobalData.TimeDiff.QuadPart;

				BothDirInformation->LastAccessTime.QuadPart	= Ext2GlobalData.TimeDiff.QuadPart + ( ( __int64 ) PtrInode->i_atime * 10000000 );
				BothDirInformation->LastWriteTime.QuadPart	= Ext2GlobalData.TimeDiff.QuadPart + ( ( __int64 )PtrInode->i_mtime * 10000000 );

				//	Getting the file type...
				BothDirInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;
				if( ! Ext2IsModeRegularFile( PtrInode->i_mode ) )
				{  
					//	Not a reqular file...
					if( Ext2IsModeDirectory( PtrInode->i_mode) )
					{
						//	Directory...
						BothDirInformation->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
					}
					else
					{
						//	Special File...
						//	Treated with respect... ;)
						//
						BothDirInformation->FileAttributes |= ( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
						//	FILE_ATTRIBUTE_DEVICE
					}
					if ( Ext2IsModeHidden( PtrInode->i_mode ) )
					{
						BothDirInformation->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
					}
					
					if ( Ext2IsModeReadOnly( PtrInode->i_mode ) )
					{
						BothDirInformation->FileAttributes |= FILE_ATTRIBUTE_READONLY;
					}
				}

				BothDirInformation->FileIndex = StartingIndexForSearch;
				BothDirInformation->FileNameLength = PtrDirEntry->name_len*2 + 2;
				BothDirInformation->ShortNameLength = 0; 
				BothDirInformation->ShortName[0] = 0;
				
				//	Copying out the name as WCHAR null terminated strings
				for( j = 0; j< PtrDirEntry->name_len ; j ++ )
				{
					//	BothDirInformation->ShortName[ j ] = PtrDirEntry->name[j];
					BothDirInformation->FileName[ j ] = PtrDirEntry->name[j];
					//	if( j < 11 )
					//		BothDirInformation->ShortName[j] = PtrDirEntry->name[j];;
				}

				/*
				if( j < 11 )
				{
					BothDirInformation->ShortNameLength = j * 2 + 2;
					BothDirInformation->ShortName[ j ] = 0;
				}
				else
				{
					BothDirInformation->ShortNameLength = 24;
					BothDirInformation->ShortName[ 11 ] = 0;
				}*/

				BothDirInformation->FileName[ j ]	= 0;
				BytesReturned += ThisBlock;
				BufferIndex += ThisBlock;

				if( !ReturnSingleEntry && ( StartingIndexForSearch < ( PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) ))
					BothDirInformation->NextEntryOffset = ThisBlock;
				else
					BothDirInformation->NextEntryOffset = 0;
				break;

			case FileDirectoryInformation:
				//	DirectoryInformation
				DebugTrace(DEBUG_TRACE_DIRINFO,   " === FileDirectoryInformation", 0 );
				ThisBlock = sizeof( FILE_DIRECTORY_INFORMATION );
				ThisBlock += PtrDirEntry->name_len*2;
				ThisBlock = Ext2QuadAlign( ThisBlock  );

				if( ( BufferIndex + ThisBlock ) > BufferLength )
				{
					//
					//	Next entry won't fit into the buffer...
					//	will have to return... 
					//	:(
					//
					if( DirectoryInformation )
						DirectoryInformation->NextEntryOffset = 0;
					if( !BytesReturned )
						RC = STATUS_NO_MORE_FILES;
					BufferUsedup = TRUE;
					break;
				}
							
				Ext2ReadInode( PtrVCB, PtrDirEntry->inode, PtrInode );
				if( !PtrInode )
				{
					try_return( RC = STATUS_UNSUCCESSFUL );
				}

				DirectoryInformation = ( PFILE_DIRECTORY_INFORMATION ) ( Buffer + ( BufferIndex ) );
				DirectoryInformation->AllocationSize.QuadPart	= PtrInode->i_blocks * 512;
				DirectoryInformation->EndOfFile.QuadPart		= PtrInode->i_size;
				DirectoryInformation->ChangeTime.QuadPart		= 0;

				DirectoryInformation->CreationTime.QuadPart	= ( __int64 ) PtrInode->i_ctime * 10000000;
				DirectoryInformation->CreationTime.QuadPart	+= Ext2GlobalData.TimeDiff.QuadPart;

				DirectoryInformation->LastAccessTime.QuadPart	= Ext2GlobalData.TimeDiff.QuadPart + ( ( __int64 ) PtrInode->i_atime * 10000000 );
				DirectoryInformation->LastWriteTime.QuadPart	= Ext2GlobalData.TimeDiff.QuadPart + ( ( __int64 )PtrInode->i_mtime * 10000000 );

				//	Getting the file type...
				DirectoryInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;
				if( ! Ext2IsModeRegularFile( PtrInode->i_mode ) )
				{  
					//	Not a reqular file...
					if( Ext2IsModeDirectory( PtrInode->i_mode) )
					{
						//	Directory...
						DirectoryInformation->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
					}
					else
					{
						//	Special File...
						//	Treated with respect... ;)
						//
						DirectoryInformation->FileAttributes |= ( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
						//	FILE_ATTRIBUTE_DEVICE
					}
					if ( Ext2IsModeHidden( PtrInode->i_mode ) )
					{
						DirectoryInformation->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
					}
					
					if ( Ext2IsModeReadOnly( PtrInode->i_mode ) )
					{
						DirectoryInformation->FileAttributes |= FILE_ATTRIBUTE_READONLY;
					}
				}

				DirectoryInformation->FileIndex = StartingIndexForSearch;
				DirectoryInformation->FileNameLength = PtrDirEntry->name_len*2 + 2;
				
				//	Copying out the name as WCHAR null terminated strings
				for( j = 0; j< PtrDirEntry->name_len ; j ++ )
				{
					DirectoryInformation->FileName[ j ] = PtrDirEntry->name[j];
				}

				DirectoryInformation->FileName[ j ]	= 0;
				BytesReturned += ThisBlock;
				BufferIndex += ThisBlock;

				if( !ReturnSingleEntry && ( StartingIndexForSearch < ( PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) ))
					DirectoryInformation->NextEntryOffset = ThisBlock;
				else
					DirectoryInformation->NextEntryOffset = 0;
				break;

			case FileFullDirectoryInformation:
				//	FullDirInformation->
				DebugTrace(DEBUG_TRACE_DIRINFO,   " === FileFullDirectoryInformation - Not handled", 0 );
				try_return( RC );
			case FileNamesInformation:
				//	NamesInformation->
				DebugTrace(DEBUG_TRACE_DIRINFO,   " === FileNamesInformation - Not handled", 0 );
				try_return( RC );
			default:
				DebugTrace(DEBUG_TRACE_DIRINFO,   " === Invalid Dir Info class - Not handled", 0 );
				try_return( RC = STATUS_INVALID_INFO_CLASS );
			}
			if( ReturnSingleEntry )
			{
				break;
			}
		}//	end of for...



		if( !BytesReturned && StartingIndexForSearch >= ( PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart) )
		{
			Ext2DeallocateUnicodeString( &PtrCCB->DirectorySearchPattern );
			PtrCCB->CurrentByteOffset.QuadPart = 0;
			if( FirstTimeQuery )
				RC = STATUS_NO_SUCH_FILE;
			else
				RC = STATUS_NO_MORE_FILES;
			try_return( RC );
		}
		else if( BytesReturned )
		{
			BothDirInformation->NextEntryOffset = 0;
		}

		try_exit:	NOTHING;
	}
	finally 
	{

		if( PtrInode )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [DirCtrl]", PtrInode );
			ExFreePool( PtrInode );
		}

		if( PtrBCB )
		{
			CcUnpinData( PtrBCB );
			PtrBCB = NULL;
		}
		
		if (PostRequest) 
		{
			if (AcquiredFCB) 
			{
				Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released in [DirCtrl]", 0);
				DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [DirCtrl]", 
					PtrReqdFCB->MainResource.ActiveCount, 
					PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
					PtrReqdFCB->MainResource.NumberOfSharedWaiters );
			}

			// Map the users buffer and then post the request.
			RC = Ext2LockCallersBuffer(PtrIrp, TRUE, BufferLength);
			ASSERT(NT_SUCCESS(RC));

			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);

		}
		else if (!(PtrIrpContext->IrpContextFlags &
							EXT2_IRP_CONTEXT_EXCEPTION)) 
		{
			if (AcquiredFCB) 
			{
				Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released [DirCtrl]", 0);
				DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [DirCtrl]", 
					PtrReqdFCB->MainResource.ActiveCount, 
					PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
					PtrReqdFCB->MainResource.NumberOfSharedWaiters );
			}

			// Complete the request.
			PtrIrp->IoStatus.Status = RC;
			PtrIrp->IoStatus.Information = BytesReturned;

			// Free up the Irp Context
			Ext2ReleaseIrpContext(PtrIrpContext);

			// complete the IRP
			IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
		}
		
		//	Flush the saved BCBs...
		//	Ext2FlushSavedBCBs ( PtrIrpContext );

	}

	return(RC);
}



/*************************************************************************
*
* Function: Ext2NotifyChangeDirectory()
*
* Description:
*	Handle the notify request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2NotifyChangeDirectory(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
#ifdef _GNU_NTIFS_
PEXTENDED_IO_STACK_LOCATION		PtrIoStackLocation,
#else
PIO_STACK_LOCATION		PtrIoStackLocation,
#endif
PFILE_OBJECT				PtrFileObject,
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB)
{
	NTSTATUS					RC = STATUS_SUCCESS;
	BOOLEAN					CompleteRequest = FALSE;
	BOOLEAN					PostRequest = FALSE;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	BOOLEAN					CanWait = FALSE;
	ULONG						CompletionFilter = 0;
	BOOLEAN					WatchTree = FALSE;
	PtrExt2VCB				PtrVCB = NULL;
	BOOLEAN					AcquiredFCB = FALSE;

	try {

		// Validate the sent-in FCB
		if ((PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) || !(PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY)) {
			// We will only allow notify requests on directories.
			RC = STATUS_INVALID_PARAMETER;
			CompleteRequest = TRUE;
		}

		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);
		CanWait = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);
	    PtrVCB = PtrFCB->PtrVCB;

		// Acquire the FCB resource shared
		DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCB Shared[DirCtrl]", 0);
		DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [DirCtrl]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
		if (!ExAcquireResourceSharedLite(&(PtrReqdFCB->MainResource), CanWait)) 
		{
			DebugTrace(DEBUG_TRACE_MISC,   "*** FCB Acquisition FAILED [DirCtrl]", 0);
			PostRequest = TRUE;
			try_return(RC = STATUS_PENDING);
		}
		AcquiredFCB = TRUE;
		DebugTrace(DEBUG_TRACE_MISC,   "*** FCB acquired [DirCtrl]", 0);

		// Obtain some parameters sent by the caller
		CompletionFilter = PtrIoStackLocation->Parameters.NotifyDirectory.CompletionFilter;
		WatchTree = (PtrIoStackLocation->Flags & SL_WATCH_TREE ? TRUE : FALSE);

		// If you wish to capture the subject context, you can do so as
		// follows:
		// {
		//		PSECURITY_SUBJECT_CONTEXT SubjectContext;
 		// 	SubjectContext = Ext2AllocatePool(PagedPool,
		//									sizeof(SECURITY_SUBJECT_CONTEXT) );
		//		SeCaptureSubjectContext(SubjectContext);
		//	}

		FsRtlNotifyFullChangeDirectory((PNOTIFY_SYNC)&(PtrVCB->NotifyIRPMutex), &(PtrVCB->NextNotifyIRP), (void *)PtrCCB,
							(PSTRING)(PtrFCB->FCBName->ObjectName.Buffer), WatchTree, FALSE, CompletionFilter, PtrIrp,
							NULL,		// Ext2TraverseAccessCheck(...) ?
							NULL);	// SubjectContext ?

		RC = STATUS_PENDING;

		try_exit:	NOTHING;

	} 
	finally 
	{

		if (PostRequest) 
		{
			// Perform appropriate post related processing here
			if (AcquiredFCB) 
			{
				Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released in DirCtrl", 0);
				DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [DirCtrl]", 
					PtrReqdFCB->MainResource.ActiveCount, 
					PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
					PtrReqdFCB->MainResource.NumberOfSharedWaiters );

				AcquiredFCB = FALSE;
			}
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
		} 
		else if (CompleteRequest) 
		{
			PtrIrp->IoStatus.Status = RC;
			PtrIrp->IoStatus.Information = 0;

			// Free up the Irp Context
			Ext2ReleaseIrpContext(PtrIrpContext);

			// complete the IRP
			IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
		} else {
			// Simply free up the IrpContext since the IRP has been queued
			Ext2ReleaseIrpContext(PtrIrpContext);
		}

		// Release the FCB resources if acquired.
		if (AcquiredFCB) 
		{
			Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
			DebugTrace(DEBUG_TRACE_MISC,  "*** FReleased in [DirCtrl]", 0);
			DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [DirCtrl]", 
					PtrReqdFCB->MainResource.ActiveCount, 
					PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
					PtrReqdFCB->MainResource.NumberOfSharedWaiters );

			AcquiredFCB = FALSE;
		}

	}

	return(RC);
}

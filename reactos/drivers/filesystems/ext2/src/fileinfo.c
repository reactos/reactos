/*************************************************************************
*
* File: fileinfo.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "set/query file information" dispatch
*	entry points.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_INFORMATION
#define			DEBUG_LEVEL						(DEBUG_TRACE_FILEINFO)


/*************************************************************************
*
* Function: Ext2FileInfo()
*
* Description:
*	The I/O Manager will invoke this routine to handle a set/query file
*	information request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*	to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2FileInfo(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "File Info Control IRP received...", 0);

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

		RC = Ext2CommonFileInfo(PtrIrpContext, Irp);

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
* Function: Ext2CommonFileInfo()
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
NTSTATUS NTAPI Ext2CommonFileInfo(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION		PtrIoStackLocation = NULL;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;
	PtrExt2VCB				PtrVCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	BOOLEAN					MainResourceAcquired = FALSE;
	BOOLEAN					VCBResourceAcquired = FALSE;
	BOOLEAN					PagingIoResourceAcquired = FALSE;
	void					*PtrSystemBuffer = NULL;
	long					BufferLength = 0;
	FILE_INFORMATION_CLASS	FunctionalityRequested;
	BOOLEAN					CanWait = FALSE;
	BOOLEAN					PostRequest = FALSE;

	try 
	{
		// First, get a pointer to the current I/O stack location.
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

		PtrFileObject = PtrIoStackLocation->FileObject;
		ASSERT(PtrFileObject);

		// Get the FCB and CCB pointers
		Ext2GetFCB_CCB_VCB_FromFileObject ( 
			PtrFileObject, &PtrFCB, &PtrCCB, &PtrVCB );

		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

		CanWait = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);

		// If the caller has opened a logical volume and is attempting to
		// query information for it as a file stream, return an error.
		if (PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) 
		{
			// This is not allowed. Caller must use get/set volume information instead.
			RC = STATUS_INVALID_PARAMETER;
			try_return(RC);
		}

		//	ASSERT(PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB);
		AssertFCB( PtrFCB );

		PtrSystemBuffer = PtrIrp->AssociatedIrp.SystemBuffer;	

		if (PtrIoStackLocation->MajorFunction == IRP_MJ_QUERY_INFORMATION) 
		{
			//	***********************
			//	Query File Information
			//	***********************

			// Now, obtain some parameters.
			DebugTrace(DEBUG_TRACE_MISC,   "Buffer length = 0x%lX[FileInfo]", BufferLength );

			BufferLength = PtrIoStackLocation->Parameters.QueryFile.Length;

			FunctionalityRequested = PtrIoStackLocation->Parameters.QueryFile.FileInformationClass;

	 		//	Read in the file inode if it hasn't already been read...
			Ext2InitializeFCBInodeInfo( PtrFCB );

			//
			// Acquire the MainResource shared 
			// except for page files...
			//
			if (!(PtrFCB->FCBFlags & EXT2_FCB_PAGE_FILE)) 
			{
				// Acquire the MainResource shared.
				DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCB Shared [FileInfo]", 0);
				DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
				if (!ExAcquireResourceSharedLite(&(PtrReqdFCB->MainResource), CanWait)) 
				{
					DebugTrace(DEBUG_TRACE_MISC,   "*** FCB acquisition FAILED [FileInfo]", 0);
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}
				MainResourceAcquired = TRUE;
				DebugTrace(DEBUG_TRACE_MISC,   "*** FCB acquired [FileInfo]", 0);
			}

			// Do whatever the caller asked us to do
			switch (FunctionalityRequested) 
			{
			case FileBasicInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileBasicInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2GetBasicInformation(PtrFCB, (PFILE_BASIC_INFORMATION)PtrSystemBuffer, &BufferLength);
				break;
			case FileStandardInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileStandardInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2GetStandardInformation(PtrFCB, (PFILE_STANDARD_INFORMATION)PtrSystemBuffer, &BufferLength);
				break;
			// Similarly, implement all of the other query information routines
			// that your FSD can support.
#if(_WIN32_WINNT >= 0x0400)
			case FileNetworkOpenInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileNetworkOpenInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2GetNetworkOpenInformation(PtrFCB, (PFILE_NETWORK_OPEN_INFORMATION)PtrSystemBuffer, &BufferLength);
				//	RC = STATUS_INVALID_PARAMETER;
				//	try_return(RC);
				break;
#endif	// _WIN32_WINNT >= 0x0400
			case FileInternalInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileInternalInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				// RC = Ext2GetInternalInformation(...);
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
				break;
			case FileEaInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileEaInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				{	
					PFILE_EA_INFORMATION EaInformation;
					EaInformation = (PFILE_EA_INFORMATION) PtrSystemBuffer;
					EaInformation->EaSize = 0;
					BufferLength = sizeof( FILE_EA_INFORMATION );
					break;
				}
				// RC = Ext2GetEaInformation(...);
				break;

			case FilePositionInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FilePositionInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				// This is fairly simple. Copy over the information from the
				// file object.
				{
					PFILE_POSITION_INFORMATION		PtrFileInfoBuffer;

					PtrFileInfoBuffer = (PFILE_POSITION_INFORMATION)PtrSystemBuffer;

					ASSERT(BufferLength >= sizeof(FILE_POSITION_INFORMATION));
					PtrFileInfoBuffer->CurrentByteOffset = PtrFileObject->CurrentByteOffset;
					// Modify the local variable for BufferLength appropriately.
					BufferLength = sizeof(FILE_POSITION_INFORMATION);
				}
				break;
			case FileStreamInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileStreamInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				// RC = Ext2GetFileStreamInformation(...);

				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
				
				break;
			case FileAllInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileAllInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				// The I/O Manager supplies the Mode, Access, and Alignment
				// information. The rest is up to us to provide.
				// Therefore, decrement the BufferLength appropriately (assuming
				// that the above 3 types on information are already in the
				// buffer)
				{

					PFILE_POSITION_INFORMATION		PtrFileInfoBuffer;
					PFILE_ALL_INFORMATION			PtrAllInfo = (PFILE_ALL_INFORMATION)PtrSystemBuffer;

					// Fill in the position information.

					PtrFileInfoBuffer = (PFILE_POSITION_INFORMATION)&(PtrAllInfo->PositionInformation);

					PtrFileInfoBuffer->CurrentByteOffset = PtrFileObject->CurrentByteOffset;

					// Modify the local variable for BufferLength appropriately.
					BufferLength = sizeof(FILE_ALL_INFORMATION);

					// Get the remaining stuff.
					if (!NT_SUCCESS(RC = Ext2GetBasicInformation(PtrFCB, (PFILE_BASIC_INFORMATION)&(PtrAllInfo->BasicInformation),
																					&BufferLength))) 
					{
						try_return(RC);
					}
					if (!NT_SUCCESS(RC = Ext2GetStandardInformation(PtrFCB, &(PtrAllInfo->StandardInformation), &BufferLength))) 
					{
						try_return(RC);
					}
					// Similarly, get all of the others ...
				}
				break;
			case FileNameInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileNameInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				//	RC = Ext2GetFullNameInformation(...);
				RC = Ext2GetFullNameInformation(PtrFCB, PtrCCB, (PFILE_NAME_INFORMATION)PtrSystemBuffer, &BufferLength);
				break;
			case FileAlternateNameInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileAlternateNameInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
				// RC = Ext2GetAltNameInformation(...);
				break;
			case FileCompressionInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "FileCompressionInformation requested for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
				// RC = Ext2GetCompressionInformation(...);
				break;

			default:
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}

			// If we completed successfully, the return the amount of information transferred.
			if (NT_SUCCESS(RC)) 
			{
				PtrIrp->IoStatus.Information = BufferLength;
			}
			else 
			{
				PtrIrp->IoStatus.Information = 0;
			}

		}
		else 
		{
			//	***********************
			//	Set File Information
			//	***********************
			ASSERT(PtrIoStackLocation->MajorFunction == IRP_MJ_SET_INFORMATION);

			DebugTrace(DEBUG_TRACE_FILEINFO,   ">>>>>>>> Set File Information <<<<<<<<< [FileInfo]", 0);


			// Now, obtain some parameters.
			FunctionalityRequested = PtrIoStackLocation->Parameters.SetFile.FileInformationClass;

			//	Check oplocks...

			//
			//	Acquire the VCB resource exclusively for 
			//	deletion, rename and link operations...
			//	To synchronize with create and cleanup operations
			//
			if ((FunctionalityRequested == FileDispositionInformation) || 
				(FunctionalityRequested == FileRenameInformation) ||
				(FunctionalityRequested == FileLinkInformation)) 
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire VCB Exclusively [FileInfo]", 0);
				DebugTraceState( " VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", PtrVCB->VCBResource.ActiveCount, PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
				if (!ExAcquireResourceExclusiveLite(&(PtrVCB->VCBResource), CanWait)) 
				{
					DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Acquisition FAILED [FileInfo]", 0);
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}
				// We have the VCB acquired exclusively.
				DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Acquired [FileInfo]", 0);
				VCBResourceAcquired = TRUE;
			}


			// Acquire the FCB exclusively at this time...
			if (!(PtrFCB->FCBFlags & EXT2_FCB_PAGE_FILE)) 
			{
				// Acquire the MainResource shared.
				DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCB Exclusively [FileInfo]", 0);
				DebugTraceState( " FCBMain AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
				if (!ExAcquireResourceExclusiveLite(&(PtrReqdFCB->MainResource), CanWait)) 
				{
					DebugTrace(DEBUG_TRACE_MISC,   "*** FCB Acquisition FAILED [FileInfo]", 0);
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}
				MainResourceAcquired = TRUE;
				DebugTrace(DEBUG_TRACE_MISC,   "*** FCB Acquired [FileInfo]", 0);
			}

			//
			//	For delete link (rename),
			//	set allocation size, and set EOF, should also acquire the paging-IO
			//	resource, thereby synchronizing with paging-IO requests. 
			//
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FileInfo]", PtrFileObject);
				
			}
			DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCBPaging Exclusively [FileInfo]", 0);
			DebugTraceState( " FCBPaging AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", PtrReqdFCB->PagingIoResource.ActiveCount, PtrReqdFCB->PagingIoResource.NumberOfExclusiveWaiters, PtrReqdFCB->PagingIoResource.NumberOfSharedWaiters );
			if (!ExAcquireResourceExclusiveLite(&(PtrReqdFCB->PagingIoResource), CanWait)) 
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** Attempt to acquire FCBPaging FAILED [FileInfo]", 0);
				PostRequest = TRUE;
				try_return(RC = STATUS_PENDING);
			}
			PagingIoResourceAcquired = TRUE;
			DebugTrace(DEBUG_TRACE_MISC,   "*** Acquired FCBPaging [FileInfo]", 0);

			// Do whatever the caller asked us to do
			switch (FunctionalityRequested) 
			{
			case FileBasicInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FileBasicInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2SetBasicInformation(PtrIrpContext, PtrFCB, PtrFileObject, (PFILE_BASIC_INFORMATION)PtrSystemBuffer);
				//	RC = STATUS_ACCESS_DENIED;
				break;
			case FileAllocationInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FileAllocationInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2SetAllocationInformation(PtrFCB, PtrCCB, PtrVCB, PtrFileObject,
																PtrIrpContext, PtrIrp, PtrSystemBuffer);
				break;
			case FileEndOfFileInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FileEndOfFileInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				// RC = Ext2SetEOF(...);
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
				break;

			case FilePositionInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FilePositionInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				// Check	if no intermediate buffering has been specified.
				// If it was specified, do not allow non-aligned set file
				// position requests to succeed.
				{
					PFILE_POSITION_INFORMATION		PtrFileInfoBuffer;

					PtrFileInfoBuffer = (PFILE_POSITION_INFORMATION)PtrSystemBuffer;

					if (PtrFileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) 
					{
						if (PtrFileInfoBuffer->CurrentByteOffset.LowPart & PtrIoStackLocation->DeviceObject->AlignmentRequirement) 
						{
							// Invalid alignment.
							try_return(RC = STATUS_INVALID_PARAMETER);
						}
					}
					PtrFileObject->CurrentByteOffset = PtrFileInfoBuffer->CurrentByteOffset;
				}
				break;

			case FileDispositionInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FileDispositionInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2SetDispositionInformation(PtrFCB, PtrCCB, PtrVCB, PtrFileObject, 
					PtrIrpContext, PtrIrp,
					(PFILE_DISPOSITION_INFORMATION)PtrSystemBuffer);
				break;

			case FileRenameInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FileRenameInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = Ext2RenameOrLinkFile( PtrFCB, PtrFileObject,	PtrIrpContext,
										PtrIrp, (PFILE_RENAME_INFORMATION)PtrSystemBuffer);
				break;
			case FileLinkInformation:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Attempt to set FileLinkInformation for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
				// When you implement your rename/link routine, be careful to
				// check the following two arguments:
				// TargetFileObject = PtrIoStackLocation->Parameters.SetFile.FileObject;
				// ReplaceExistingFile = PtrIoStackLocation->Parameters.SetFile.ReplaceIfExists;
				//
				// The TargetFileObject argument is a pointer to the "target
				// directory" file object obtained during the "create" routine
				// invoked by the NT I/O Manager with the SL_OPEN_TARGET_DIRECTORY
				// flag specified. Remember that it is quite possible that if the
				// rename/link is contained within a single directory, the target
				// and source directories will be the same.
				// The ReplaceExistingFile argument should be used by you to
				// determine if the caller wishes to replace the target (if it
				// currently exists) with the new link/renamed file. If this
				// value is FALSE, and if the target directory entry (being
				// renamed-to, or the target of the link) exists, you should
				// return a STATUS_OBJECT_NAME_COLLISION error to the caller.

				// RC = Ext2RenameOrLinkFile(PtrFCB, PtrCCB, PtrFileObject,	PtrIrpContext,
				//						PtrIrp, (PFILE_RENAME_INFORMATION)PtrSystemBuffer);

				// Once you have completed the rename/link operation, do not
				// forget to notify any "notify IRPs" about the actions you have
				// performed.
				// An example is if you renamed across directories, you should
				// report that a new entry was added with the FILE_ACTION_ADDED
				// action type. The actual modification would then be reported
				// as either FILE_NOTIFY_CHANGE_FILE_NAME (if a file was renamed)
				// or FILE_NOTIFY_CHANGE_DIR_NAME (if a directory was renamed).
				
				break;
			default:
				DebugTrace(DEBUG_TRACE_FILEINFO,   "Unrecoganised SetFileInformation code for %S", PtrFCB->FCBName->ObjectName.Buffer );
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}
		}

		try_exit:	NOTHING;
	}
	finally
	{
		if (PagingIoResourceAcquired) 
		{
			Ext2ReleaseResource(&(PtrReqdFCB->PagingIoResource));
			DebugTrace(DEBUG_TRACE_MISC,  "*** FCBPaging Released [FileInfo]", 0);
			DebugTraceState( " FCBPaging AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", 
				PtrReqdFCB->PagingIoResource.ActiveCount, 
				PtrReqdFCB->PagingIoResource.NumberOfExclusiveWaiters, 
				PtrReqdFCB->PagingIoResource.NumberOfSharedWaiters );

			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FileInfo]", PtrFileObject);
			}

			PagingIoResourceAcquired = FALSE;
		}

		if (MainResourceAcquired) 
		{
			Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
			DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released [FileInfo]", 0);
			DebugTraceState( " FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", 
				PtrReqdFCB->MainResource.ActiveCount, 
				PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
				PtrReqdFCB->MainResource.NumberOfSharedWaiters );
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FileInfo]", PtrFileObject);
			}
			
			MainResourceAcquired = FALSE;
		}

		if (VCBResourceAcquired) 
		{
			Ext2ReleaseResource(&(PtrVCB->VCBResource));
			DebugTrace(DEBUG_TRACE_MISC,  "*** VCB Released [FileInfo]", 0);
			DebugTraceState( " VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [FileInfo]", 
				PtrVCB->VCBResource.ActiveCount, 
				PtrVCB->VCBResource.NumberOfExclusiveWaiters, 
				PtrVCB->VCBResource.NumberOfSharedWaiters );

			VCBResourceAcquired = FALSE;
		}

		// Post IRP if required
		if (PostRequest) 
		{

			// Since, the I/O Manager gave us a system buffer, we do not
			// need to "lock" anything.

			// Perform the post operation which will mark the IRP pending
			// and will return STATUS_PENDING back to us
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);

		} 
		else 
		{

			// Can complete the IRP here if no exception was encountered
			if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_EXCEPTION)) {
				PtrIrp->IoStatus.Status = RC;

				// Free up the Irp Context
				Ext2ReleaseIrpContext(PtrIrpContext);
	
				// complete the IRP
				IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
			}
		} // can we complete the IRP ?
	} // end of "finally" processing
	
	// DbgPrint( "\n	=== File Info IRP returning --> RC : 0x%lX   Bytes: 0x%lX", PtrIrp->IoStatus.Status, PtrIrp->IoStatus.Information );
	
	return(RC);
}


/*************************************************************************
*
* Function: Ext2GetBasicInformation()
*
* Description:
*	Return some time-stamps and file attributes to the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2GetBasicInformation(
PtrExt2FCB					PtrFCB,
PFILE_BASIC_INFORMATION		PtrBuffer,
long						*PtrReturnedLength )
{
	NTSTATUS			RC = STATUS_SUCCESS;

	try 
	{
		if (*PtrReturnedLength < sizeof(FILE_BASIC_INFORMATION)) 
		{
			try_return(RC = STATUS_BUFFER_OVERFLOW);
		}

		// Zero out the supplied buffer.
		RtlZeroMemory(PtrBuffer, sizeof(FILE_BASIC_INFORMATION));

		// Get information from the FCB.
		PtrBuffer->CreationTime		= PtrFCB->CreationTime;
		PtrBuffer->LastAccessTime	= PtrFCB->LastAccessTime;
		PtrBuffer->LastWriteTime	= PtrFCB->LastWriteTime;
		PtrBuffer->ChangeTime		= PtrFCB->LastWriteTime;
		
		// Now fill in the attributes.
		PtrBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
		if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_DIRECTORY ) )
		{
			PtrBuffer->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		}
		else if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_SPECIAL_FILE ) )
		{
			//	Special File...
			//	Treated with respect... ;)
			//
			PtrBuffer->FileAttributes |= ( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY) ;
		}

		if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_HIDDEN_FILE  ) )
		{
			PtrBuffer->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		}
		
		if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_READ_ONLY ) )
		{
			PtrBuffer->FileAttributes |= FILE_ATTRIBUTE_READONLY;
		}
		
		try_exit: NOTHING;
	} 
	finally 
	{
		if (NT_SUCCESS(RC)) 
		{
			// Return the amount of information filled in.
			*PtrReturnedLength = sizeof(FILE_BASIC_INFORMATION);
		}
	}
	return(RC);
}

NTSTATUS NTAPI Ext2GetStandardInformation(
PtrExt2FCB					PtrFCB,

PFILE_STANDARD_INFORMATION	PtrStdInformation,
long						*PtrReturnedLength )
{

	NTSTATUS			RC = STATUS_SUCCESS;


	try 
	{
		if (*PtrReturnedLength < sizeof( FILE_STANDARD_INFORMATION )) 
		{
			try_return(RC = STATUS_BUFFER_OVERFLOW);
		}

		// Zero out the supplied buffer.
		RtlZeroMemory(PtrStdInformation, sizeof(FILE_STANDARD_INFORMATION));
	
				
		PtrStdInformation->AllocationSize	= PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize;
		PtrStdInformation->EndOfFile		= PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize;
		PtrStdInformation->DeletePending	= Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_DELETE_ON_CLOSE);
		PtrStdInformation->Directory		= Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_DIRECTORY );
		PtrStdInformation->NumberOfLinks	= PtrFCB->LinkCount;

		try_exit: NOTHING;
	} 
	finally 
	{
		if (NT_SUCCESS(RC)) 
		{
			// Return the amount of information filled in.
			*PtrReturnedLength = sizeof( FILE_STANDARD_INFORMATION );
		}
	}
	return(RC);
}

NTSTATUS NTAPI Ext2GetNetworkOpenInformation(
	PtrExt2FCB						PtrFCB,
	PFILE_NETWORK_OPEN_INFORMATION	PtrNetworkOpenInformation,
	long							*PtrReturnedLength )
{

	NTSTATUS			RC = STATUS_SUCCESS;

	try 
	{
		if (*PtrReturnedLength < sizeof( FILE_NETWORK_OPEN_INFORMATION )) 
		{
			try_return(RC = STATUS_BUFFER_OVERFLOW);
		}

		// Zero out the supplied buffer.
		RtlZeroMemory(PtrNetworkOpenInformation, sizeof(FILE_NETWORK_OPEN_INFORMATION));

		PtrNetworkOpenInformation->CreationTime		= PtrFCB->CreationTime;
		PtrNetworkOpenInformation->LastAccessTime	= PtrFCB->LastAccessTime;
		PtrNetworkOpenInformation->LastWriteTime	= PtrFCB->LastWriteTime;
		PtrNetworkOpenInformation->ChangeTime		= PtrFCB->LastWriteTime;
		PtrNetworkOpenInformation->AllocationSize	= PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize;
		PtrNetworkOpenInformation->EndOfFile		= PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize;

		// Now fill in the attributes.
		PtrNetworkOpenInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;

		if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_DIRECTORY ) )
		{
			PtrNetworkOpenInformation->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		}
		else if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_SPECIAL_FILE ) )
		{
			//	Special File...	
			//	Treated with respect... ;)
			//
			PtrNetworkOpenInformation->FileAttributes |= ( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY) ;
		}
		if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_HIDDEN_FILE  ) )
		{
			PtrNetworkOpenInformation->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		}
		
		if ( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_READ_ONLY ) )
		{
			PtrNetworkOpenInformation->FileAttributes |= FILE_ATTRIBUTE_READONLY;
		}
		try_exit: NOTHING;
	} 
	finally 
	{
		if (NT_SUCCESS(RC)) 
		{
			// Return the amount of information filled in.
			*PtrReturnedLength = sizeof( FILE_NETWORK_OPEN_INFORMATION );
		}
	}
	return(RC);
}


NTSTATUS NTAPI Ext2GetFullNameInformation(
	PtrExt2FCB				PtrFCB,
	PtrExt2CCB				PtrCCB,
	PFILE_NAME_INFORMATION	PtrNameInformation,
	long					*PtrReturnedLength )
{

	NTSTATUS			RC = STATUS_SUCCESS;

	try 
	{
		if (*PtrReturnedLength < 
			(long)( sizeof(FILE_NAME_INFORMATION) + PtrCCB->AbsolutePathName.Length) ) 
		{
			try_return(RC = STATUS_BUFFER_OVERFLOW);
		}

		// Zero out the supplied buffer.
		RtlZeroMemory(PtrNameInformation, sizeof( FILE_NAME_INFORMATION ) );
		
		if( PtrCCB->AbsolutePathName.Length )
		{
			RtlCopyMemory(
				PtrNameInformation->FileName,		//	Destination,
				PtrCCB->AbsolutePathName.Buffer,	//	Source,
				PtrCCB->AbsolutePathName.Length );	//	Length

			PtrNameInformation->FileNameLength = PtrCCB->AbsolutePathName.Length;
			try_return(RC = STATUS_SUCCESS);
		}
		else
		{
			try_return(RC = STATUS_INVALID_PARAMETER);
		}
		
		try_exit: NOTHING;
	} 
	finally 
	{
		if (NT_SUCCESS(RC)) 
		{
			// Return the amount of information filled in.
			*PtrReturnedLength =
				sizeof( FILE_NAME_INFORMATION ) +
				PtrCCB->AbsolutePathName.Length;
		}
	}
	return(RC);
}
/*************************************************************************
*
* Function: Ext2SetBasicInformation()
*
* Description:
*	Set some time-stamps and file attributes supplied by the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2SetBasicInformation(
	PtrExt2IrpContext			PtrIrpContext,
	PtrExt2FCB					PtrFCB,
	PFILE_OBJECT				PtrFileObject,
	PFILE_BASIC_INFORMATION		PtrFileInformation )
{
	NTSTATUS			RC = STATUS_SUCCESS;

	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;
	AssertVCB( PtrVCB );
	
	//	BOOLEAN			CreationTimeChanged = FALSE;
	//	BOOLEAN			AttributesChanged = FALSE;

//	return STATUS_INVALID_PARAMETER;

	try 
	{
		EXT2_INODE		Inode;
		RtlZeroMemory(&Inode, sizeof( EXT2_INODE ));

		if( NT_SUCCESS( Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
		{
			//
			//	Update time stamps in the inode
			//	and in the FCB...
			//
			if( PtrFileInformation->CreationTime.QuadPart )
			{
				PtrFCB->CreationTime.QuadPart = PtrFileInformation->CreationTime.QuadPart;
				Inode.i_ctime = (ULONG) ( (PtrFCB->CreationTime.QuadPart
						- Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );

			}
			if( PtrFileInformation->LastAccessTime.QuadPart )
			{
				PtrFCB->LastAccessTime.QuadPart = PtrFileInformation->LastAccessTime.QuadPart;
				Inode.i_atime = (ULONG) ( (PtrFCB->LastAccessTime.QuadPart
						- Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );

			}
			if( PtrFileInformation->LastWriteTime.QuadPart )
			{
				PtrFCB->LastWriteTime.QuadPart = PtrFileInformation->LastWriteTime.QuadPart;
				Inode.i_mtime = (ULONG) ( (PtrFCB->LastWriteTime.QuadPart
						- Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );
			}

			// Now come the attributes.
			if (PtrFileInformation->FileAttributes) 
			{
				if (PtrFileInformation->FileAttributes & FILE_ATTRIBUTE_READONLY ) 
				{
					//	Turn off the write bits...
					Ext2SetModeReadOnly( Inode.i_mode );
				}
				if (PtrFileInformation->FileAttributes & FILE_ATTRIBUTE_HIDDEN ) 
				{
					//	Turn off the read and write bits...
					Ext2SetModeHidden( Inode.i_mode );

				}
				if (PtrFileInformation->FileAttributes & FILE_ATTRIBUTE_SYSTEM ) 
				{
					//	Just turn off the read and write bits...
					//	No special field to indicate that this is a system file...
					Ext2SetModeReadOnly( Inode.i_mode );
					Ext2SetModeHidden( Inode.i_mode );
				}
			}

			//	Updating the inode...
			Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode );
		}
	
		if (PtrFileInformation->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) 
		{
			Ext2SetFlag(PtrFileObject->Flags, FO_TEMPORARY_FILE);
		} 
		else 
		{
			Ext2ClearFlag(PtrFileObject->Flags, FO_TEMPORARY_FILE);
		}
	} 
	finally 
	{
		;
	}
	return(RC);
}


/*************************************************************************
*
* Function: Ext2SetDispositionInformation()
*
* Description:
*	Mark/Unmark a file for deletion.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2SetDispositionInformation(
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB,
PtrExt2VCB					PtrVCB,
PFILE_OBJECT				PtrFileObject,
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
PFILE_DISPOSITION_INFORMATION	PtrBuffer)
{
	NTSTATUS			RC = STATUS_SUCCESS;

	try 
	{
		if (!PtrBuffer->DeleteFile) 
		{
			// "un-delete" the file.
			Ext2ClearFlag(PtrFCB->FCBFlags, EXT2_FCB_DELETE_ON_CLOSE);
			PtrFileObject->DeletePending = FALSE;
			try_return(RC);
		}

		// Do some checking to see if the file can even be deleted.

		if (PtrFCB->FCBFlags & EXT2_FCB_DELETE_ON_CLOSE) 
		{
			// All done!
			try_return(RC);
		}

		if (PtrFCB->FCBFlags & EXT2_FCB_READ_ONLY) 
		{
			try_return(RC = STATUS_CANNOT_DELETE);
		}

		if (PtrVCB->VCBFlags & EXT2_VCB_FLAGS_VOLUME_READ_ONLY) 
		{
			try_return(RC = STATUS_CANNOT_DELETE);
		}

		// An important step is to check if the file stream has been
		// mapped by any process. The delete cannot be allowed to proceed
		// in this case.
		if (!MmFlushImageSection(&(PtrFCB->NTRequiredFCB.SectionObject), MmFlushForDelete)) 
		{
			try_return(RC = STATUS_CANNOT_DELETE);
		}

		// Disallow deletion of either a root
		// directory or a directory that is not empty.
		if( PtrFCB->INodeNo == EXT2_ROOT_INO )
		{
			try_return(RC = STATUS_CANNOT_DELETE);
		}

		if (PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY) 
		{
			if (!Ext2IsDirectoryEmpty(PtrFCB, PtrCCB, PtrIrpContext)) 
			{
					try_return(RC = STATUS_DIRECTORY_NOT_EMPTY);
			}
		}

		// Set a flag to indicate that this directory entry will become history
		// at cleanup.
		Ext2SetFlag(PtrFCB->FCBFlags, EXT2_FCB_DELETE_ON_CLOSE);
		PtrFileObject->DeletePending = TRUE;

		try_exit: NOTHING;
	}
	finally 
	{
		;
	}
	return(RC);
}



/*************************************************************************
*
* Function: Ext2SetAllocationInformation()
*
* Description:
*	Mark/Unmark a file for deletion.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2SetAllocationInformation(
PtrExt2FCB					PtrFCB,
PtrExt2CCB					PtrCCB,
PtrExt2VCB					PtrVCB,
PFILE_OBJECT				PtrFileObject,
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
PFILE_ALLOCATION_INFORMATION	PtrBuffer)
{
	NTSTATUS		RC = STATUS_SUCCESS;
	BOOLEAN			TruncatedFile = FALSE;
	BOOLEAN			ModifiedAllocSize = FALSE;

	try 
	{
			
		// Are we increasing the allocation size?
		if (RtlLargeIntegerLessThan(
				PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize,
				PtrBuffer->AllocationSize)) 
		{
			ULONG CurrentSize;
			ULONG LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
			
			for( CurrentSize = 0; CurrentSize < PtrBuffer->AllocationSize.QuadPart; CurrentSize += LogicalBlockSize )
			{
				Ext2AddBlockToFile( PtrIrpContext, PtrVCB, PtrFCB, PtrFileObject, FALSE );
			}
			ModifiedAllocSize = TRUE;
		} 
		else if (RtlLargeIntegerGreaterThan(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize,
																PtrBuffer->AllocationSize)) 
		{
			// This is the painful part. See if the VMM will allow us to proceed.
			// The VMM will deny the request if:
			// (a) any image section exists OR
			// (b) a data section exists and the size of the user mapped view
			//		 is greater than the new size
			// Otherwise, the VMM should allow the request to proceed.
			if (!MmCanFileBeTruncated(&(PtrFCB->NTRequiredFCB.SectionObject), &(PtrBuffer->AllocationSize))) 
			{
				// VMM said no way!
				try_return(RC = STATUS_USER_MAPPED_FILE);
			}

			if( !Ext2TruncateFileAllocationSize( PtrIrpContext, PtrFCB, PtrFileObject, &PtrBuffer->AllocationSize) )
			{
				//	This will do until I figure out a better error message code ;)
				RC = STATUS_INSUFFICIENT_RESOURCES;

			}

			ModifiedAllocSize = TRUE;
			TruncatedFile = TRUE;
		}

		try_exit:

			// This is a good place to check if we have performed a truncate
			// operation. If we have perform a truncate (whether we extended
			// or reduced file size), you should update file time stamps.

			// Last, but not the lease, you must inform the Cache Manager of file size changes.
			if (ModifiedAllocSize && NT_SUCCESS(RC)) 
			{
				// Update the FCB Header with the new allocation size.
				PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize = PtrBuffer->AllocationSize;

				// If we decreased the allocation size to less than the
				// current file size, modify the file size value.
				// Similarly, if we decreased the value to less than the
				// current valid data length, modify that value as well.
				if (TruncatedFile) 
				{
					if (RtlLargeIntegerLessThan(PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize, PtrBuffer->AllocationSize)) 
					{
						// Decrease the file size value.
						PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize = PtrBuffer->AllocationSize;
					}

					if (RtlLargeIntegerLessThan(PtrFCB->NTRequiredFCB.CommonFCBHeader.ValidDataLength, PtrBuffer->AllocationSize)) 
					{
						// Decrease the valid data length value.
						PtrFCB->NTRequiredFCB.CommonFCBHeader.ValidDataLength = PtrBuffer->AllocationSize;
					}
				}


				// If the FCB has not had caching initiated, it is still valid
				// for you to invoke the NT Cache Manager. It is possible in such
				// situations for the call to be no'oped (unless some user has
				// mapped in the file)

				// NOTE: The invocation to CcSetFileSizes() will quite possibly
				//	result in a recursive call back into the file system.
				//	This is because the NT Cache Manager will typically
				//	perform a flush before telling the VMM to purge pages
				//	especially when caching has not been initiated on the
				//	file stream, but the user has mapped the file into
				//	the process' virtual address space.
				CcSetFileSizes(PtrFileObject, (PCC_FILE_SIZES)&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize));

				// Inform any pending IRPs (notify change directory).
			}

	} 
	finally 
	{
		;
	}
	return(RC);
}

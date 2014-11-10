/*************************************************************************
*
* File: write.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "Write" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_WRITE

#define			DEBUG_LEVEL						(DEBUG_TRACE_WRITE)


/*************************************************************************
*
* Function: Ext2Write()
*
* Description:
*	The I/O Manager will invoke this routine to handle a write
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
NTSTATUS NTAPI Ext2Write(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
    PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "Write IRP Received...", 0);

	//	Ext2BreakPoint();

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

		RC = Ext2CommonWrite(PtrIrpContext, Irp);
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
* Function: Ext2CommonWrite()
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
NTSTATUS NTAPI Ext2CommonWrite(
	PtrExt2IrpContext			PtrIrpContext,
	PIRP						PtrIrp)
{

	NTSTATUS				RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION		PtrIoStackLocation = NULL;
	LARGE_INTEGER			ByteOffset;
	uint32					WriteLength = 0;
	uint32					NumberBytesWritten = 0;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;
	PtrExt2VCB				PtrVCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	PERESOURCE				PtrResourceAcquired = NULL;
	void					*PtrSystemBuffer = NULL;

	BOOLEAN					CompleteIrp = TRUE;
	BOOLEAN					PostRequest = FALSE;

	BOOLEAN					CanWait = FALSE;
	BOOLEAN					PagingIo = FALSE;
	BOOLEAN					NonBufferedIo = FALSE;
	BOOLEAN					SynchronousIo = FALSE;
	BOOLEAN					IsThisADeferredWrite = FALSE;
	BOOLEAN					WritingAtEndOfFile = FALSE;

	EXT2_IO_RUN				*PtrIoRuns = NULL;
			
	PBCB		PtrPinnedSIndirectBCB = NULL;
	PBCB		PtrPinnedDIndirectBCB = NULL;
	PBCB		PtrPinnedTIndirectBCB = NULL;
	
	//	Used to cache the Single Indirect blocks pointed to by 
	//	the Double Indirect block
	PEXT2_SIBLOCKS			PtrDIArray = NULL;
	ULONG					DIArrayCount = 0;

	//	Used to cache the Single Indirect blocks pointed to by 
	//	the Triple Indirect block
	PEXT2_SIBLOCKS			PtrTIArray = NULL;
	ULONG					TIArrayCount = 0;


	try 
	{
		// First, get a pointer to the current I/O stack location
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

		PtrFileObject = PtrIoStackLocation->FileObject;
		ASSERT(PtrFileObject);

  		// If this happens to be a MDL write complete request, then
		// allocated MDL can be freed. 
		if(PtrIoStackLocation->MinorFunction & IRP_MN_COMPLETE) 
		{
			// Caller wants to tell the Cache Manager that a previously
			// allocated MDL can be freed.
			Ext2MdlComplete(PtrIrpContext, PtrIrp, PtrIoStackLocation, FALSE);
			// The IRP has been completed.
			CompleteIrp = FALSE;
			try_return(RC = STATUS_SUCCESS);
		}

		// If this is a request at IRQL DISPATCH_LEVEL, then post the request 
		if (PtrIoStackLocation->MinorFunction & IRP_MN_DPC) 
		{
			CompleteIrp = FALSE;
			PostRequest = TRUE;
			try_return(RC = STATUS_PENDING);
		}


		// Get the FCB and CCB pointers
		Ext2GetFCB_CCB_VCB_FromFileObject ( 
			PtrFileObject, &PtrFCB, &PtrCCB, &PtrVCB );


		// Get some of the parameters supplied to us
		ByteOffset = PtrIoStackLocation->Parameters.Write.ByteOffset;
		WriteLength = PtrIoStackLocation->Parameters.Write.Length;

		CanWait = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);
		PagingIo = ((PtrIrp->Flags & IRP_PAGING_IO) ? TRUE : FALSE);
		NonBufferedIo = ((PtrIrp->Flags & IRP_NOCACHE) ? TRUE : FALSE);
		SynchronousIo = ((PtrFileObject->Flags & FO_SYNCHRONOUS_IO) ? TRUE : FALSE);

		if( PtrFCB && PtrFCB->FCBName && PtrFCB->FCBName->ObjectName.Length && PtrFCB->FCBName->ObjectName.Buffer )
		{
			DebugTrace(DEBUG_TRACE_FILE_NAME,   " === Write File Name : -%S-", PtrFCB->FCBName->ObjectName.Buffer );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_FILE_NAME,   " === Write File Name : -null-", 0);
		}
		
		DebugTrace( DEBUG_TRACE_SPECIAL,   "  ->ByteCount           = 0x%8lx", PtrIoStackLocation->Parameters.Read.Length);
		DebugTrace( DEBUG_TRACE_SPECIAL,   "  ->ByteOffset.LowPart  = 0x%8lx", PtrIoStackLocation->Parameters.Read.ByteOffset.LowPart);
		
		if( CanWait )
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->Can Wait ", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->Can't Wait ", 0 );
		}
		
		if( PagingIo )
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->Paging Io ", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->Not Paging Io", 0 );
		}

		if( SynchronousIo )
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->SynchronousIo ", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->ASynchronousIo ", 0 );
		}

		if( NonBufferedIo )
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->NonBufferedIo", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "  ->BufferedIo", 0 );
		}
		// Check at this point whether the file object being
		// used for write really did have write permission requested when the
		// create/open operation was performed. 
		// Don't do this for paging io...

		if (WriteLength == 0) 
		{
			// a 0 byte write can be immediately succeeded
			try_return();
		}

		// Is this a write of the volume itself ?
		if ( ( !PtrFCB && PtrVCB ) || PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) 
		{
			//
			//		>>>>>>>>>>>>>>>>>>		VOLUME WRITE	  <<<<<<<<<<<<<<
			//

			//	Validate the offset and length first...
			//	.......................................

			// Acquire the volume resource exclusively
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Write]", PtrFileObject);
			}

			if( PagingIo )
			{
				//	This is Paging IO...

				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** Attempting to acquire VCBPaging Exclusively [Write]", 0);
				DebugTraceState( "VCBPaging       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Write]", PtrVCB->PagingIoResource.ActiveCount, PtrVCB->PagingIoResource.NumberOfExclusiveWaiters, PtrVCB->PagingIoResource.NumberOfSharedWaiters );
				
				if( !ExAcquireResourceExclusiveLite( &( PtrVCB->PagingIoResource ), FALSE ) )
				{
					// post the request to be processed in the context of a worker thread
					DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** VCBPaging Acquisition FAILED [Write]", 0);
					CompleteIrp = FALSE;
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}

				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** VCBPaging Acquired [Write]", 0);
				PtrResourceAcquired = &(PtrVCB->PagingIoResource);
			}
			else
			{
				//	This is not Paging IO...

				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** Attempting to acquire VCB Exclusively [Write]", 0);
				DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Write]", PtrVCB->VCBResource.ActiveCount, 
					PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
				
				if( !ExAcquireResourceExclusiveLite( &(PtrVCB->VCBResource), FALSE ) )
				{
					// post the request to be processed in the context of a worker thread
					DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** VCB Acquisition FAILED [Write]", 0);
					CompleteIrp = FALSE;
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}

				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** VCB Acquired [Write]", 0);
				PtrResourceAcquired = &(PtrVCB->VCBResource);
			}
			
			// Validate the caller supplied offset here
			if( !PagingIo )
			{
				if( PtrVCB->CommonVCBHeader.AllocationSize.QuadPart < ByteOffset.QuadPart )
				{
					//	Write extending beyond the end of volume.
					//	Deny access...
					//
					RC = STATUS_END_OF_FILE;
					NumberBytesWritten = 0;
					try_return();
				}
			}

			// Lock the callers buffer
			if (!NT_SUCCESS(RC = Ext2LockCallersBuffer(PtrIrp, TRUE, WriteLength))) 
			{
				try_return();
			}

			// Forward the request to the lower level driver
			if( PagingIo || NonBufferedIo )
			{
				DebugTrace(DEBUG_TRACE_WRITE_DETAILS,  "[Volume Write] PagingIo or NonBufferedIo ", 0);
				CompleteIrp = FALSE;

				//
				//	Do the write operation...
				//	Send down the IRP to the lower level driver...
				//
				//	Returned Informations and Status will be set by the lower level driver...
				//	The IRP will also be completed by the lower level driver...

				RC = Ext2PassDownSingleReadWriteIRP (
						PtrIrpContext, PtrIrp, PtrVCB, 
						ByteOffset, WriteLength, SynchronousIo );

				try_return();
			}
			else
			{
				PBCB		PtrBCB = NULL;
				PVOID		PtrBuffer = NULL;

				DebugTrace(DEBUG_TRACE_READ_DETAILS,  "[Volume Write] BufferedIo ", 0);
								//
				//	Let the cache manager worry about this write...
				//	Pinned access should have been initiated.
				//	But checking anyway...
				//
				ASSERT( PtrVCB->PtrStreamFileObject );
				ASSERT( PtrVCB->PtrStreamFileObject->PrivateCacheMap );

				CcPreparePinWrite( 
					PtrVCB->PtrStreamFileObject,
					&ByteOffset,
					WriteLength,
					FALSE,			//	Don't Zero...
					TRUE,			//	Can Wait...
					&PtrBCB,
					&PtrBuffer);

				//	Do the write now...
				//	Write to the Pinned buffer...
				//	Cache Manager will do the disk write...
				RtlCopyBytes( PtrBuffer, PtrSystemBuffer, WriteLength );
				//	CcSetDirtyPinnedData( PtrBCB, NULL );
				CcUnpinData( PtrBCB );
				PtrBuffer = NULL;
				PtrBCB = NULL;

				NumberBytesWritten = WriteLength;

				//	Go ahead and complete the IRP...
			}
			try_return();
		}

      
		IsThisADeferredWrite = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_DEFERRED_WRITE) ? TRUE : FALSE);

		if (!NonBufferedIo) 
		{
			/****************************************************************************
			if (!CcCanIWrite(PtrFileObject, WriteLength, CanWait, IsThisADeferredWrite)) 
			{
				// Cache Manager and/or the VMM does not want us to perform
				// the write at this time. Post the request.
				Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_DEFERRED_WRITE);
				CcDeferWrite( PtrFileObject, Ext2DeferredWriteCallBack, PtrIrpContext, PtrIrp, WriteLength, IsThisADeferredWrite);
				CompleteIrp = FALSE;
				try_return( RC = STATUS_PENDING );
			}
			****************************************************************************/
		}

		// If the write request is directed to a page file 
		// send the request directly to the disk
		// driver. For requests directed to a page file, you have to trust
		// that the offsets will be set correctly by the VMM. You should not
		// attempt to acquire any FSD resources either.
		if (PtrFCB->FCBFlags & EXT2_FCB_PAGE_FILE) 
		{
			IoMarkIrpPending(PtrIrp);

			// You will need to set a completion routine before invoking a lower level driver.
			//	Forward request directly to disk driver.
			// Ext2PageFileIo(PtrIrpContext, PtrIrp);

			CompleteIrp = FALSE;

			try_return(RC = STATUS_PENDING);
		}

		// Check whether this write operation is targeted
		// to a directory object...

		if (PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY) 
		{
			//
			//	Is this a write a result of 
			//	cached directory manipulation operatio
			//	by the FSD itself?
			//
			if( PagingIo )
			{
				//	Yep! Allow it to proceed...
			}
			else
			{
				//	Nope... User initiated directory writes are not allowed!
				//	Fail this request...
				RC = STATUS_INVALID_DEVICE_REQUEST;
				try_return();
			}
		}

		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

		//
		//	Synchronizing with other reads and writes...
		//	Acquire the appropriate FCB resource exclusively
		//
		if (PagingIo) 
		{
			// Try to acquire the FCB PagingIoResource exclusively
			DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** Attempting to acquire FCBpaging Exclusively [Write]", 0);

			if (!ExAcquireResourceExclusiveLite(&(PtrReqdFCB->PagingIoResource), CanWait)) 
			{
				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** FCBpaging Acquisition FAILED [Write]", 0);
				CompleteIrp = FALSE;
				PostRequest = TRUE;
				try_return(RC = STATUS_PENDING);
			}
			// Remember the resource that was acquired
			DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** FCBpaging Acquired [Write]", 0);
			PtrResourceAcquired = &(PtrReqdFCB->PagingIoResource);
		} 
		else 
		{
			// Try to acquire the FCB MainResource exclusively
			DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** Attempting to acquire FCB Exclusively [Write]", 0);
			if (!ExAcquireResourceExclusiveLite(&(PtrReqdFCB->MainResource), CanWait)) 
			{
				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** FCB Acquisition FAILED [Write]", 0);
				CompleteIrp = FALSE;
				PostRequest = TRUE;
				try_return(RC = STATUS_PENDING);
			}
			// Remember the resource that was acquired
			DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE,   "*** FCB Acquired [Write]", 0);
			PtrResourceAcquired = &(PtrReqdFCB->MainResource);
		}

		// Validate start offset and length supplied.
		// Here is a special check that determines whether the caller wishes to
		// begin the write at current end-of-file (whatever the value of that
		// offset might be)
		if ((ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE) && (ByteOffset.HighPart == 0xFFFFFFFF)) 
		{
			WritingAtEndOfFile = TRUE;
			ByteOffset.QuadPart = PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart;
		}

		//
		//	If this is a Non Cached io and if caching has been initiated,
		//	Flush and purge the cache 
		//
		if (NonBufferedIo && !PagingIo && (PtrReqdFCB->SectionObject.DataSectionObject != NULL)) 
		{
			// Flush and then attempt to purge the cache
			CcFlushCache(&(PtrReqdFCB->SectionObject), &ByteOffset, WriteLength, &(PtrIrp->IoStatus));
			// If the flush failed, return error to the caller
			if (!NT_SUCCESS(RC = PtrIrp->IoStatus.Status)) 
			{
				try_return();
			}

			// Attempt the purge and ignore the return code
			CcPurgeCacheSection( &(PtrReqdFCB->SectionObject), (WritingAtEndOfFile ? &(PtrReqdFCB->CommonFCBHeader.FileSize) : &(ByteOffset)),
										WriteLength, FALSE);
			// We are finished with our flushing and purging
		}

		if (!PagingIo) 
		{
			//	Insert code to perform the check here ...
			//	
			//	if (!Ext2CheckForByteLock(PtrFCB, PtrCCB, PtrIrp,
			//		PtrCurrentIoStackLocation)) 
			//	{
			//		try_return(RC = STATUS_FILE_LOCK_CONFLICT);
			//	}
		}

		//	Read in the File inode...
		Ext2InitializeFCBInodeInfo( PtrFCB );

		if (!PagingIo) 
		{
			LARGE_INTEGER  CurrentTime;
			KeQuerySystemTime( &CurrentTime );
			PtrFCB->LastAccessTime.QuadPart = CurrentTime.QuadPart;
			PtrFCB->LastWriteTime.QuadPart = CurrentTime.QuadPart;
		}

		{
			//
			// Validate start offset and length supplied.
			//
			ULONG		LogicalBlockSize = 0;
			LONGLONG	NoOfNewBlocksRequired = 0;
			LONGLONG	NoOfBytesRequired = 0;
			LONGLONG	i;
			BOOLEAN		ZeroOut = FALSE;
			LARGE_INTEGER	StartOffsetForZeroing;
			LARGE_INTEGER	EndOffsetForZeroing;


			LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
		
			if ( ByteOffset.QuadPart + WriteLength > PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart )
			{
				if( PagingIo )
				{
					//
					//	A paging request never modifies the file size...
					//
					if( ByteOffset.QuadPart 
						>= PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart )
					{
						//	Page request for writing outside the file...
						//	No op this IRP by completing it...
						//
						try_return();
					}
					if( ByteOffset.QuadPart + WriteLength
						> PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart )
					{
						//	Page request for writing outside the file alocation size...
						//	Truncate the write size so that it is within the allocation limit...
						//
						WriteLength = (ULONG) (PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart - ByteOffset.QuadPart);
					}
				}
				else
				{
					
					//	Starting offset is > file size
					//	Allocate new blocks?
					NoOfBytesRequired = ByteOffset.QuadPart + WriteLength 
							- PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart;

					if( NoOfBytesRequired )
					{
						NoOfNewBlocksRequired = Ext2Align64( NoOfBytesRequired, LogicalBlockSize ) 
							/ LogicalBlockSize;
						for( i = 0; i < NoOfNewBlocksRequired ; i++ )
						{
							Ext2AddBlockToFile( PtrIrpContext, PtrVCB, PtrFCB, PtrFileObject, FALSE );
						}
				
						ZeroOut = TRUE;

						if( PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart < ByteOffset.QuadPart )
						{
							//	   Curr EOF --> |				     | <--New EOF
							//	-----------------------------------------------
							//  |				|		 |///////////|		  |
							//	| File Contents |  Free  |// Write //|  Free  | <- End of Allocation   
							//	|				|		 |///////////|        |
							//	-----------------------------------------------
							//	                |				     | 

							//	Write is beyond the current end of file...
							//	This will create a hole...
							//	Will have to zero this out...

							//	Start offset is the Current File size
							StartOffsetForZeroing = PtrReqdFCB->CommonFCBHeader.FileSize;
							//	End offset is the point at which this write is going to start
							EndOffsetForZeroing = ByteOffset;
						}
						else
						{
							//			   Curr EOF --> |		| <--New EOF
							//	------------------------------------------
							//	|				|///////////////|		 |
							//	| File Contents |//// Write ////|  Free  | <- End of Allocation
							//	|				|///////////////|		 |
							//	------------------------------------------
							//	                        |       | 

							//	Just zero out the end of the file 
							//	not covered by the file size

							//	Start offset is the New File size
							StartOffsetForZeroing.QuadPart = 
								ByteOffset.QuadPart + WriteLength;
							//	End offset is the New Allocation size
							EndOffsetForZeroing.QuadPart = PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart;
						}
					}

					PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart = 
						ByteOffset.QuadPart + WriteLength;				
					
					ASSERT( PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart <= PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart );

					Ext2UpdateFileSize( PtrIrpContext, PtrFileObject, PtrFCB );

					try
					{
						//
						//	Zero the blocks out...
						//	This routine can be used even if caching has not been initiated...
						//
						if( ZeroOut == TRUE && StartOffsetForZeroing.QuadPart != EndOffsetForZeroing.QuadPart )
						{
							CcZeroData( PtrFileObject, 
								&StartOffsetForZeroing,
								&EndOffsetForZeroing, 
								FALSE );
							
							if( EndOffsetForZeroing.QuadPart != PtrReqdFCB->CommonFCBHeader.AllocationSize.QuadPart )
							{
								//	Also zero out the file tip...
								CcZeroData( PtrFileObject, 
									&PtrReqdFCB->CommonFCBHeader.FileSize,
									&PtrReqdFCB->CommonFCBHeader.AllocationSize, 
									FALSE );
							}
						}
					}
					finally
					{
						//	Swallow an exceptions that are raised...
					}
				}
			}
		}

		//
		//	Branch here for cached vs non-cached I/O
		//
		if (!NonBufferedIo) 
		{

			//	The caller wishes to perform cached I/O. 
			//	Initiate caching if it hasn't been done already...
			if (PtrFileObject->PrivateCacheMap == NULL) 
			{
				CcInitializeCacheMap(PtrFileObject, (PCC_FILE_SIZES)(&(PtrReqdFCB->CommonFCBHeader.AllocationSize)),
					FALSE,		// We will not utilize pin access for this file
					&(Ext2GlobalData.CacheMgrCallBacks), // callbacks
					PtrCCB);		// The context used in callbacks
			}

			// Check and see if this request requires a MDL returned to the caller
			if (PtrIoStackLocation->MinorFunction & IRP_MN_MDL) 
			{
				// Caller does want a MDL returned. Note that this mode
				// implies that the caller is prepared to block
				CcPrepareMdlWrite(PtrFileObject, &ByteOffset, WriteLength, &(PtrIrp->MdlAddress), &(PtrIrp->IoStatus));
				NumberBytesWritten = PtrIrp->IoStatus.Information;
				RC = PtrIrp->IoStatus.Status;

				try_return();
			}

			// This is a regular run-of-the-mill cached I/O request. Let the
			// Cache Manager worry about it!

			// First though, we need a buffer pointer (address) that is valid
			PtrSystemBuffer = Ext2GetCallersBuffer(PtrIrp);
			ASSERT(PtrSystemBuffer);
			if ( !CcCopyWrite(PtrFileObject, &(ByteOffset), WriteLength, CanWait, PtrSystemBuffer)) 
			{
				// The caller was not prepared to block and data is not immediately
				// available in the system cache
				CompleteIrp = FALSE;
				PostRequest = TRUE;
				// Mark Irp Pending ...
				try_return(RC = STATUS_PENDING);
			} 
			else 
			{
				// We have the data
				PtrIrp->IoStatus.Status = RC;
				PtrIrp->IoStatus.Information = NumberBytesWritten = WriteLength;
			}
		}
		else //	NonBuffered or Paged IO
		{

			ULONG		Start = 0;
			ULONG		End = 0;
			ULONG		LogicalBlockIndex = 0;
			ULONG		BytesRemaining = 0;
			ULONG		BytesWrittenSoFar = 0;
			ULONG		LeftOver = 0;
			ULONG		LogicalBlockSize = 0;
			ULONG		PhysicalBlockSize = 0;
			ULONG		Index = 0;

			LONGLONG	SingleIndirectBlockSize	= 0;
			LONGLONG	DoubleIndirectBlockSize = 0;
			LONGLONG	TripleIndirectBlockSize = 0;
			LONGLONG	DirectBlockSize = 0;

			LONGLONG	NoOfDirectBlocks ;
			LONGLONG	NoOfSingleIndirectBlocks ;
			LONGLONG	NoOfDoubleIndirectBlocks ;
			LONGLONG	NoOfTripleIndirectBlocks ;

			ULONG * PtrPinnedSIndirectBlock = NULL;
			ULONG * PtrPinnedDIndirectBlock = NULL;
			ULONG * PtrPinnedTIndirectBlock = NULL;

			//	Used when reading a Triple Indirect Block...
			LONGLONG		FirstCachedDIBlockOffset = 0;

			//	Used when reading a Double Indirect Block...
			LONGLONG		FirstCachedSIBlockOffset = 0;

			DebugTrace(DEBUG_TRACE_WRITE_DETAILS,  "[File Write] Paging IO or NonBufferedIo ", 0);

			//	Calculating where the write should start from...
			LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
			PhysicalBlockSize = PtrVCB->TargetDeviceObject->SectorSize;

			NoOfDirectBlocks =  EXT2_NDIR_BLOCKS ;
			NoOfSingleIndirectBlocks = LogicalBlockSize / sizeof( ULONG );
			NoOfDoubleIndirectBlocks = NoOfSingleIndirectBlocks * LogicalBlockSize / sizeof( ULONG );
			NoOfTripleIndirectBlocks = NoOfDoubleIndirectBlocks * LogicalBlockSize / sizeof( ULONG );

			DirectBlockSize =  LogicalBlockSize * NoOfDirectBlocks;
			SingleIndirectBlockSize	= LogicalBlockSize * NoOfSingleIndirectBlocks;
			DoubleIndirectBlockSize = LogicalBlockSize * NoOfDoubleIndirectBlocks ;
			TripleIndirectBlockSize	= LogicalBlockSize * NoOfTripleIndirectBlocks;

			LogicalBlockIndex = (ULONG)( ByteOffset.QuadPart / LogicalBlockSize);

			if( ( ByteOffset.QuadPart + WriteLength ) > DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
			{
				//
				//	Handle Triple indirect blocks?
				//	A Pop up will do for now...
				//
				UNICODE_STRING ErrorMessage;
				Ext2CopyWideCharToUnicodeString( &ErrorMessage, L"Triple indirect blocks not supported as yet. - Ext2.sys" );
				DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "@@@@@@@@  Triple indirect blocks need to be written to! \n@@@@@@@@  This is not supported as yet!", 0);
				/* REACTOS FIXME */
				IoRaiseInformationalHardError(
						/* IO_ERR_DRIVER_ERROR */(NTSTATUS)0xC0040004L,
						&ErrorMessage,
						KeGetCurrentThread( ) );

				Ext2DeallocateUnicodeString( &ErrorMessage );
				RC = STATUS_INSUFFICIENT_RESOURCES;
				try_return();
			}
			
			if( ( ByteOffset.QuadPart + WriteLength ) > DirectBlockSize &&
				( ByteOffset.QuadPart < DirectBlockSize + SingleIndirectBlockSize ) )
			{
				LARGE_INTEGER VolumeByteOffset;

				//
				//	Indirect Blocks required...
				//	Read in the single indirect blocks...
				//
				DebugTrace(DEBUG_TRACE_WRITE_DETAILS,   "Reading in some Indirect Blocks", 0);

				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_NDIR_BLOCKS ] * LogicalBlockSize;

				//
				//	Asking the cache manager to oblige by pinning the single indirect block...
				//
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   LogicalBlockSize,
				   CanWait,
				   &PtrPinnedSIndirectBCB,
				   (PVOID*)&PtrPinnedSIndirectBlock )) 
				{
					CompleteIrp = FALSE;
					PostRequest = TRUE;
				
					// Mark Irp Pending ...
					IoMarkIrpPending( PtrIrp );
					RC = STATUS_PENDING;
					try_return();
					DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data", 0);
				}
			}
			if( ( ByteOffset.QuadPart + WriteLength ) > DirectBlockSize + SingleIndirectBlockSize &&
				( ByteOffset.QuadPart ) < DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
			{
				//
				//	Double Indirect Blocks required...
				//	Read in the double indirect blocks...
				//

				LONGLONG		StartIndirectBlock;
				LONGLONG		EndIndirectBlock;

				

				LARGE_INTEGER VolumeByteOffset;

				DebugTrace(DEBUG_TRACE_MISC,   "Reading in some Double Indirect Blocks", 0);

				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;

				//
				//	Asking the cache manager to oblige by pinning the double indirect block...
				//
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
                   &VolumeByteOffset,
                   LogicalBlockSize,
                   CanWait,
                   &PtrPinnedDIndirectBCB,
                   (PVOID*)&PtrPinnedDIndirectBlock )) 
				{
					CompleteIrp = FALSE;
					PostRequest = TRUE;
				
					// Mark Irp Pending ...
					IoMarkIrpPending( PtrIrp );
					RC = STATUS_PENDING;
					try_return();
					DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
				}

				//	So far so good...
				//	Now determine the single indirect blocks that will have to be read in...
				if( ByteOffset.QuadPart >= DirectBlockSize + SingleIndirectBlockSize )
				{
					//	Request doesnot require any single indirect or direct blocks
					StartIndirectBlock = ByteOffset.QuadPart - (DirectBlockSize + SingleIndirectBlockSize);
					StartIndirectBlock = StartIndirectBlock / LogicalBlockSize;
					StartIndirectBlock = StartIndirectBlock / NoOfSingleIndirectBlocks;
				}
				else
				{
					StartIndirectBlock = 0;
				}

				FirstCachedSIBlockOffset = (NoOfSingleIndirectBlocks*(StartIndirectBlock+1)) + NoOfDirectBlocks;

				if( ByteOffset.QuadPart + WriteLength >= 
					DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize)
				{
					EndIndirectBlock  = DoubleIndirectBlockSize;
				}
				else
				{
					EndIndirectBlock = ByteOffset.QuadPart + WriteLength - 
									   (DirectBlockSize + SingleIndirectBlockSize);
				}
				EndIndirectBlock = Ext2Align64( EndIndirectBlock, LogicalBlockSize )/LogicalBlockSize ;
				EndIndirectBlock = Ext2Align64( EndIndirectBlock, NoOfSingleIndirectBlocks )/NoOfSingleIndirectBlocks;
	
				DIArrayCount = (ULONG)(EndIndirectBlock - StartIndirectBlock);

				PtrDIArray = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( DIArrayCount * sizeof( EXT2_SIBLOCKS	) ) );
				{
					ULONG	i;

					for( i = 0; i < DIArrayCount; i++ )
					{
						VolumeByteOffset.QuadPart =  PtrPinnedDIndirectBlock[StartIndirectBlock+i] * LogicalBlockSize;
						if (!CcMapData( PtrVCB->PtrStreamFileObject,
						   &VolumeByteOffset,
						   LogicalBlockSize,
						   CanWait,
						   &PtrDIArray[i].PtrBCB,
						   (PVOID*)&PtrDIArray[i].PtrSIBlocks)) 
						{
							CompleteIrp = FALSE;
							PostRequest = TRUE;
							IoMarkIrpPending( PtrIrp );
							DIArrayCount = i;
							try_return(RC = STATUS_PENDING);

							DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
						}
					}
				}
			}

/*			{
				//
				//	Double Indirect Blocks required...
				//	Read in the double indirect blocks...
				//

				LONGLONG		StartIndirectBlock;
				LONGLONG		EndIndirectBlock;

				LARGE_INTEGER VolumeByteOffset;

				DebugTrace(DEBUG_TRACE_MISC,   "Reading in some Double Indirect Blocks", 0);

				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;

				//
				//	Asking the cache manager to oblige by pinning the double indirect block...
				//
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
                   &VolumeByteOffset,
                   LogicalBlockSize,
                   CanWait,
                   &PtrPinnedDIndirectBCB,
                   (PVOID*)&PtrPinnedDIndirectBlock )) 
				{
					CompleteIrp = FALSE;
					PostRequest = TRUE;
				
					// Mark Irp Pending ...
					IoMarkIrpPending( PtrIrp );
					RC = STATUS_PENDING;
					try_return();
					DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
				}

				//	So far so good...
				//	Now determine the single indirect blocks that will have to be read in...
				if( ByteOffset.QuadPart >= DirectBlockSize + SingleIndirectBlockSize )
				{
					//	Request doesnot require any single indirect or direct blocks
					StartIndirectBlock = ByteOffset.QuadPart - (DirectBlockSize + SingleIndirectBlockSize);
					StartIndirectBlock = StartIndirectBlock / LogicalBlockSize;
					StartIndirectBlock = StartIndirectBlock / NoOfSingleIndirectBlocks;
				}
				else
				{
					StartIndirectBlock = 0;
				}

				if( ByteOffset.QuadPart + WriteLength >= 
					DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize)
				{
					EndIndirectBlock  = DoubleIndirectBlockSize;
				}
				else
				{
					EndIndirectBlock = ByteOffset.QuadPart + WriteLength - 
									   (DirectBlockSize + SingleIndirectBlockSize);
					if( EndIndirectBlock % LogicalBlockSize )
					{
						EndIndirectBlock += LogicalBlockSize;
					}
					EndIndirectBlock = EndIndirectBlock / LogicalBlockSize;
					if( EndIndirectBlock % NoOfSingleIndirectBlocks)
					{
						EndIndirectBlock += NoOfSingleIndirectBlocks;
					}
					EndIndirectBlock = EndIndirectBlock / NoOfSingleIndirectBlocks;
				}
				DIArrayCount = (ULONG)(EndIndirectBlock - StartIndirectBlock);
				PtrDIArray = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( DIArrayCount * sizeof( EXT2_SIBLOCKS	) ) );
				{
					ULONG	i;

					for( i = 0; i < DIArrayCount; i++ )
					{
						VolumeByteOffset.QuadPart =  PtrPinnedDIndirectBlock[StartIndirectBlock+i] * LogicalBlockSize;
						if (!CcMapData( PtrVCB->PtrStreamFileObject,
						   &VolumeByteOffset,
						   LogicalBlockSize,
						   CanWait,
						   &PtrDIArray[i].PtrBCB,
						   (PVOID*)&PtrDIArray[i].PtrSIBlocks)) 
						{
							CompleteIrp = FALSE;
							PostRequest = TRUE;
							IoMarkIrpPending( PtrIrp );
							DIArrayCount = i;
							try_return(RC = STATUS_PENDING);

							DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
						}
					}
				}
			}
*/
			if( ( ByteOffset.QuadPart + WriteLength ) > DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
			{
				//
				//	Triple Indirect Blocks required...
				//	Read in the triple indirect blocks...
				//
				LONGLONG		StartTIndirectBlock;
				LONGLONG		EndTIndirectBlock;

				LONGLONG		StartDIndirectBlock;
				LONGLONG		EndDIndirectBlock;
				LONGLONG		StartIndirectBlock;
				LONGLONG		EndIndirectBlock;

				LONGLONG		ByteOffsetTillHere = 0;

				PBCB	TempDIBCB;
				LONG*	TempDIBuffer;

				ULONG TIArrayIndex = 0;

				LARGE_INTEGER VolumeByteOffset;

				DebugTrace(DEBUG_TRACE_MISC,   "Reading in some Triple Indirect Blocks", 0);

				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_TIND_BLOCK ] * LogicalBlockSize;

				DebugTrace(DEBUG_TRACE_TRIPLE,   "ByteOffset = 0x%I64X", ByteOffset );
				DebugTrace(DEBUG_TRACE_TRIPLE,   "WriteLength = 0x%lX", WriteLength );
				DebugTrace(DEBUG_TRACE_TRIPLE,   "EXT2_TIND_BLOCK = 0x%lX", PtrFCB->IBlock[ EXT2_TIND_BLOCK ] );
				//
				//	Asking the cache manager to oblige by pinning the triple indirect block...
				//
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
                   &VolumeByteOffset,
                   LogicalBlockSize,
                   CanWait,
                   &PtrPinnedTIndirectBCB,
                   (PVOID*)&PtrPinnedTIndirectBlock )) 
				{
					CompleteIrp = FALSE;
					PostRequest = TRUE;
				
					// Mark Irp Pending ...
					IoMarkIrpPending( PtrIrp );
					RC = STATUS_PENDING;
					try_return();
					DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
				}

				//	Determine the no of BCBs that need to be saved...
				if( ByteOffset.QuadPart >= DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
				{
					StartTIndirectBlock = ByteOffset.QuadPart;
				}
				else
				{
					StartTIndirectBlock = DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize;
				}
				EndTIndirectBlock = ByteOffset.QuadPart + WriteLength;
				TIArrayCount = (ULONG)( (EndTIndirectBlock - StartTIndirectBlock) / SingleIndirectBlockSize ) + 2;

				
				PtrTIArray = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( TIArrayCount * sizeof( EXT2_SIBLOCKS	) ) );

				//	Now determine the double indirect blocks that will have to be read in...
				if( ByteOffset.QuadPart >= DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
				{
					//	Request doesnot require any single indirect or direct blocks
					StartDIndirectBlock = ByteOffset.QuadPart - (DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize);
					StartDIndirectBlock = StartDIndirectBlock / LogicalBlockSize;
					StartDIndirectBlock = StartDIndirectBlock / NoOfDoubleIndirectBlocks;

					ByteOffsetTillHere = DirectBlockSize + SingleIndirectBlockSize + (DoubleIndirectBlockSize*(StartDIndirectBlock+1)) ;
					//FirstCachedDIBlockOffset = ByteOffset.QuadPart / LogicalBlockSize;
					FirstCachedDIBlockOffset = ByteOffsetTillHere / LogicalBlockSize;
				}
				else
				{
					ByteOffsetTillHere = DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize;
					FirstCachedDIBlockOffset = ByteOffsetTillHere / LogicalBlockSize;
					StartDIndirectBlock = 0;
				}

				DebugTrace(DEBUG_TRACE_TRIPLE,   "ByteOffsetTillHere = 0x%lX", ByteOffsetTillHere );

				EndDIndirectBlock  = ByteOffset.QuadPart + WriteLength - 
					(DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize);
				EndDIndirectBlock = Ext2Align64( EndDIndirectBlock, LogicalBlockSize ) / LogicalBlockSize ;
				EndDIndirectBlock = Ext2Align64( EndDIndirectBlock, NoOfDoubleIndirectBlocks ) / NoOfDoubleIndirectBlocks;

				{
					//	Reading in the necessary double indirect bocks...
					ULONG	i;
					LONGLONG Count = EndDIndirectBlock-StartDIndirectBlock;

					for( i = 0; i < Count; i++, ByteOffsetTillHere += DoubleIndirectBlockSize)
					{
						VolumeByteOffset.QuadPart =  PtrPinnedTIndirectBlock[StartDIndirectBlock+i] * LogicalBlockSize;
						
						DebugTrace(DEBUG_TRACE_TRIPLE,   "Double VolOffset = 0x%I64X", VolumeByteOffset );

						if( !CcMapData( PtrVCB->PtrStreamFileObject,
						   &VolumeByteOffset,
						   LogicalBlockSize,
						   CanWait,
						   &TempDIBCB,
						   (PVOID*)&TempDIBuffer) )
						{
							CompleteIrp = FALSE;
							PostRequest = TRUE;
							IoMarkIrpPending( PtrIrp );
							try_return(RC = STATUS_PENDING);
							DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
						}
					
						if( ByteOffset.QuadPart > ByteOffsetTillHere)
						{
							StartIndirectBlock = ByteOffset.QuadPart - (ByteOffsetTillHere);
							StartIndirectBlock = StartIndirectBlock / LogicalBlockSize;
							StartIndirectBlock = StartIndirectBlock / NoOfSingleIndirectBlocks;

							if( TIArrayIndex == 0 )
							{
								FirstCachedDIBlockOffset += StartIndirectBlock * NoOfSingleIndirectBlocks;
							}
						}
						else
						{
							StartIndirectBlock = 0;
						}

						if( ByteOffset.QuadPart + WriteLength >= ByteOffsetTillHere + DoubleIndirectBlockSize)
						{
							EndIndirectBlock  = DoubleIndirectBlockSize;
						}
						else
						{
							EndIndirectBlock = ByteOffset.QuadPart + WriteLength - ByteOffsetTillHere;
						}
						EndIndirectBlock = Ext2Align64( EndIndirectBlock, LogicalBlockSize )/LogicalBlockSize ;
						EndIndirectBlock = Ext2Align64( EndIndirectBlock, NoOfSingleIndirectBlocks )/NoOfSingleIndirectBlocks;
					
						{
							ULONG	i;
								
							for( i = 0; i < (EndIndirectBlock - StartIndirectBlock); i++ )
							{
								VolumeByteOffset.QuadPart =  TempDIBuffer[StartIndirectBlock+i] * LogicalBlockSize;
								DebugTrace(DEBUG_TRACE_TRIPLE,   "Single VolOffset = 0x%I64X", VolumeByteOffset );

								if (!CcMapData( PtrVCB->PtrStreamFileObject,
								   &VolumeByteOffset,
								   LogicalBlockSize,
								   CanWait,
								   &PtrTIArray[ TIArrayIndex ].PtrBCB,
								   (PVOID*)&PtrTIArray[ TIArrayIndex ].PtrSIBlocks)) 
								{
									CompleteIrp = FALSE;
									PostRequest = TRUE;
									IoMarkIrpPending( PtrIrp );
									DIArrayCount = i;
									try_return(RC = STATUS_PENDING);

									DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
								}
								TIArrayIndex++;
							}
						}
						CcUnpinData( TempDIBCB );
						TempDIBCB = NULL;
						TempDIBuffer = NULL;
					}
				}
				TIArrayCount = TIArrayIndex;
				
				DebugTrace(DEBUG_TRACE_TRIPLE,   "TIArrayCount = %ld", TIArrayCount );
				DebugTrace(DEBUG_TRACE_TRIPLE,   "FirstCachedDIBlockOffset = 0x%lX", FirstCachedDIBlockOffset );
			}

			//	
			//	Allocating memory for IO Runs
			//
			Index = ( (WriteLength - 2) / LogicalBlockSize + 2 );
			PtrIoRuns = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( Index * sizeof( EXT2_IO_RUN) )  );
			

			Start = (ULONG) ( ByteOffset.QuadPart - (LogicalBlockSize * LogicalBlockIndex) );
			BytesRemaining = (ULONG)( LogicalBlockSize * (LogicalBlockIndex +1) - ByteOffset.QuadPart );

			if( WriteLength > BytesRemaining )
				End = Start + BytesRemaining;
			else
				End = Start + WriteLength;
			BytesWrittenSoFar = 0;

			Index = 0;
			DebugTrace(DEBUG_TRACE_WRITE_DETAILS, "\nDetermining the write IRPs that have to be passed down...", 0);
			
			while( 1 )
			{
				BytesWrittenSoFar += (End-Start);
				if( LogicalBlockIndex < EXT2_NDIR_BLOCKS )
				{
					//	Direct Block
					PtrIoRuns[ Index ].LogicalBlock = PtrFCB->IBlock[ LogicalBlockIndex ];
				}
				else if( LogicalBlockIndex < (NoOfSingleIndirectBlocks + NoOfDirectBlocks) )
				{
					//	Single Indirect Block
					PtrIoRuns[ Index ].LogicalBlock = PtrPinnedSIndirectBlock[ LogicalBlockIndex - EXT2_NDIR_BLOCKS ];
				}
				else if( LogicalBlockIndex < (NoOfDoubleIndirectBlocks + NoOfSingleIndirectBlocks + NoOfDirectBlocks)  )
				{
					LONGLONG BlockNo;
					LONGLONG IBlockIndex;
					LONGLONG BlockIndex;

					BlockNo		= LogicalBlockIndex - FirstCachedSIBlockOffset;
					IBlockIndex	= BlockNo / NoOfSingleIndirectBlocks;
					BlockIndex	= BlockNo % NoOfSingleIndirectBlocks;

					//	Double Indirect Block
					PtrIoRuns[ Index ].LogicalBlock = 
						PtrDIArray[ IBlockIndex ].PtrSIBlocks[ BlockIndex ];
				}
				else
				{
					//	Triple Indirect Block
					LONGLONG BlockNo;
					LONGLONG IBlockIndex;
					LONGLONG BlockIndex;
					BlockNo		= LogicalBlockIndex - FirstCachedDIBlockOffset;
					IBlockIndex	= BlockNo / NoOfSingleIndirectBlocks;
					BlockIndex	= BlockNo % NoOfSingleIndirectBlocks;

					DbgPrint( "\nBlock No : 0x%I64X   IBlockIndex = 0x%I64X   BlockIndex = 0x%I64X", BlockNo, IBlockIndex, BlockIndex);

					if( IBlockIndex >= TIArrayCount )
					{
						Ext2BreakPoint();
					}
					if( BlockIndex >= LogicalBlockSize )
					{
						Ext2BreakPoint();
					}

					PtrIoRuns[ Index ].LogicalBlock = PtrTIArray[ IBlockIndex ].PtrSIBlocks[ BlockIndex ];
					DbgPrint( "LogicalBlock = 0x%lX", PtrIoRuns[ Index ].LogicalBlock );
				}

				if( PtrIoRuns[ Index ].LogicalBlock == 0 )
				{
					//
					//	Something is wrong...
					//
					Ext2BreakPoint();
					break;

				}
				PtrIoRuns[ Index ].StartOffset = Start;
				PtrIoRuns[ Index ].EndOffset = End;
				PtrIoRuns[ Index ].PtrAssociatedIrp = NULL;

				DebugTrace( DEBUG_TRACE_WRITE_DETAILS, "  Index = (%ld)", LogicalBlockIndex );
				DebugTrace( DEBUG_TRACE_WRITE_DETAILS, "  Logical Block = (0x%lX)", PtrFCB->IBlock[ LogicalBlockIndex ] );
				DebugTrace( DEBUG_TRACE_WRITE_DETAILS, "  Start = (0x%lX)", Start );
				DebugTrace( DEBUG_TRACE_WRITE_DETAILS, "  End = (0x%lX)  ", End );
				DebugTrace( DEBUG_TRACE_WRITE_DETAILS, "  Bytes written (0x%lX)", BytesWrittenSoFar );
				 
				

				if( BytesWrittenSoFar >= WriteLength )
					break;
				LogicalBlockIndex++;
				Start = 0;
				LeftOver = WriteLength - BytesWrittenSoFar;
				if( LeftOver > LogicalBlockSize )
					End = LogicalBlockSize;
				else
					End = LeftOver;
				//	Loop over to make the write request...
				Index++;
			}
			
			//
			//	Unpin the Indirect Blocks
			//
			if( PtrPinnedSIndirectBCB )
			{
				CcUnpinData( PtrPinnedSIndirectBCB );
				PtrPinnedSIndirectBCB = NULL;
				PtrPinnedSIndirectBlock = NULL;
			}
			if( PtrPinnedDIndirectBCB )
			{
				CcUnpinData( PtrPinnedDIndirectBCB );
				PtrPinnedDIndirectBCB = NULL;
				PtrPinnedDIndirectBlock = NULL;
			}
			//
			//	Pass down Associated IRPs to the Target Device Driver...
			//
			DebugTrace( DEBUG_TRACE_WRITE_DETAILS, "Passing down the Write IRPs to the disk driver...", 0 );

			RC = Ext2PassDownMultiReadWriteIRP( PtrIoRuns, Index+1, WriteLength, PtrIrpContext, PtrFCB, SynchronousIo );
			
			//
			//	Irp will be completed automatically 
			//	when all the Associated IRPs are completed
			//
			if( RC == STATUS_SUCCESS || RC == STATUS_PENDING )
			{
				CompleteIrp = FALSE;	
			}
			try_return();
		}

		try_exit:	NOTHING;

	}
	finally 
	{
		if ( PtrIoRuns )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Write]", PtrIoRuns );
			ExFreePool( PtrIoRuns );
		}
		if( PtrPinnedSIndirectBCB )
		{
			CcUnpinData( PtrPinnedSIndirectBCB );
			PtrPinnedSIndirectBCB = NULL;
		}
		if( PtrPinnedDIndirectBCB )
		{
			CcUnpinData( PtrPinnedDIndirectBCB );
			PtrPinnedDIndirectBCB = NULL;
		}
		if ( PtrDIArray )
		{
			ULONG	i;
			for( i = 0; i < DIArrayCount; i++ )
			{
				CcUnpinData( PtrDIArray->PtrBCB );
			}
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Read]", PtrDIArray );
			ExFreePool( PtrDIArray );
			PtrDIArray = NULL;
		}			
		// Release resources ...
		//
		if (PtrResourceAcquired) 
		{
			Ext2ReleaseResource(PtrResourceAcquired);
			
			DebugTrace(DEBUG_TRACE_RESOURCE_RELEASE,   "*** Resource Released [Write]", 0);
			DebugTraceState( "Resource     AC:0x%LX   SW:0x%LX   EX:0x%LX   [Write]", 
				PtrResourceAcquired->ActiveCount, 
				PtrResourceAcquired->NumberOfExclusiveWaiters, 
				PtrResourceAcquired->NumberOfSharedWaiters );
			
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Write]", PtrFileObject);
			}
			if( PtrVCB && PtrResourceAcquired == &(PtrVCB->VCBResource) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCB Released [Write]", 0);
			}
			else if( PtrVCB && PtrResourceAcquired == &(PtrVCB->PagingIoResource ) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCBPaging Released [Write]", 0);
			}
			else if( PtrReqdFCB && PtrResourceAcquired == &(PtrReqdFCB->PagingIoResource) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Paging Resource Released [Write]", 0);
			}
			else if(PtrReqdFCB && PtrResourceAcquired == &(PtrReqdFCB->MainResource) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Resource Released [Write]", 0);
			}
			else
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** Unknown Resource Released [Write]", 0);
			}

			PtrResourceAcquired = NULL;
		}

		if (PostRequest) 
		{
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
		} 
		else if ( CompleteIrp && !(RC == STATUS_PENDING)) 
		{
			// For synchronous I/O, the FSD must maintain the current byte offset
			// Do not do this however, if I/O is marked as paging-io
			if (SynchronousIo && !PagingIo && NT_SUCCESS(RC)) 
			{
				PtrFileObject->CurrentByteOffset = RtlLargeIntegerAdd(ByteOffset,
				RtlConvertUlongToLargeInteger((unsigned long)NumberBytesWritten));
			}

			// If the write completed successfully and this was not a paging-io
			// operation, set a flag in the CCB that indicates that a write was
			// performed and that the file time should be updated at cleanup
			if (NT_SUCCESS(RC) && !PagingIo) 
			{
				Ext2SetFlag(PtrCCB->CCBFlags, EXT2_CCB_MODIFIED);
			}

			// If the file size was changed, set a flag in the FCB indicating that
			// this occurred.

			// If the request failed, and we had done some nasty stuff like
			// extending the file size (including informing the Cache Manager
			// about the new file size), and allocating on-disk space etc., undo
			// it at this time.

			// Can complete the IRP here if no exception was encountered
			if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_EXCEPTION)) 
			{
				PtrIrp->IoStatus.Status = RC;
				PtrIrp->IoStatus.Information = NumberBytesWritten;
 
				// complete the IRP
				IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
			}

			// Free up the Irp Context
			Ext2ReleaseIrpContext(PtrIrpContext);

		} // can we complete the IRP ?
		else
		{
			// Free up the Irp Context
			Ext2ReleaseIrpContext(PtrIrpContext);
		}
	} // end of "finally" processing
	return(RC);
}


/*************************************************************************
*
* Function: Ext2DeferredWriteCallBack()
*
* Description:
*	Invoked by the cache manager in the context of a worker thread.
*	Typically, you can simply post the request at this point (just
*	as you would have if the original request could not block) to
*	perform the write in the context of a system worker thread.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2DeferredWriteCallBack (
void			*Context1,			// Should be PtrIrpContext
void			*Context2 )			// Should be PtrIrp
{
	// You should typically simply post the request to your internal
	// queue of posted requests (just as you would if the original write
	// could not be completed because the caller could not block).
	// Once you post the request, return from this routine. The write
	// will then be retried in the context of a system worker thread
}

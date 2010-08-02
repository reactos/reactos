/*************************************************************************
*
* File: read.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "Read" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_READ

#define			DEBUG_LEVEL						(DEBUG_TRACE_READ)


/*************************************************************************
*
* Function: Ext2Read()
*
* Description:
*	The I/O Manager will invoke this routine to handle a read
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
NTSTATUS NTAPI Ext2Read(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;
	
	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "Read IRP Received...", 0);

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

		RC = Ext2CommonRead(PtrIrpContext, Irp, TRUE);

	}
	except ( Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation() ) ) 
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
* Function: Ext2CommonRead()
*
* Description:
*	The actual work is performed here. This routine may be invoked in one
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
NTSTATUS NTAPI Ext2CommonRead(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
BOOLEAN						FirstAttempt )
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION		PtrIoStackLocation = NULL;
	LARGE_INTEGER			ByteOffset;
	uint32					ReadLength = 0, TruncatedReadLength = 0;
	uint32					NumberBytesRead = 0;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;
	PtrExt2VCB				PtrVCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	PERESOURCE				PtrResourceAcquired = NULL;
	PVOID					PtrSystemBuffer = NULL;
	PVOID					PtrPinnedReadBuffer = NULL;

	BOOLEAN					CompleteIrp = TRUE;
	BOOLEAN					PostRequest = FALSE;

	BOOLEAN					CanWait = FALSE;
	BOOLEAN					PagingIo = FALSE;
	BOOLEAN					NonBufferedIo = FALSE;
	BOOLEAN					SynchronousIo = FALSE;
	BOOLEAN					ReadTruncated = FALSE;

	//	Used to cache the Single Indirect blocks pointed to by 
	//	the Double Indirect block
	PEXT2_SIBLOCKS			PtrDIArray = NULL;
	ULONG					DIArrayCount = 0;

	//	Used to cache the Single Indirect blocks pointed to by 
	//	the Triple Indirect block
	PEXT2_SIBLOCKS			PtrTIArray = NULL;
	ULONG					TIArrayCount = 0;

	EXT2_IO_RUN	*			PtrIoRuns = NULL;

	ULONG	Start;
	ULONG	End;
	ULONG	LogicalBlockIndex;
	ULONG	BytesRemaining;
	ULONG	BytesReadSoFar;
	ULONG	LeftOver;
	ULONG	LogicalBlockSize;
	ULONG	PhysicalBlockSize;

	PBCB PtrPinnedSIndirectBCB = NULL;
	PBCB PtrPinnedDIndirectBCB = NULL;
	PBCB PtrPinnedTIndirectBCB = NULL;

	int Index;

	try 
	{
		try{
		// First, get a pointer to the current I/O stack location
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

  		// If this happens to be a MDL read complete request, then
		// there is not much processing that the FSD has to do.
		if (PtrIoStackLocation->MinorFunction & IRP_MN_COMPLETE) 
		{
			// Caller wants to tell the Cache Manager that a previously
			// allocated MDL can be freed.
			Ext2MdlComplete(PtrIrpContext, PtrIrp, PtrIoStackLocation, TRUE);
			// The IRP has been completed.
			CompleteIrp = FALSE;
			try_return(RC = STATUS_SUCCESS);
		}

		// If this is a request at IRQL DISPATCH_LEVEL, then post
		// the request (your FSD may choose to process it synchronously
		// if you implement the support correctly; obviously you will be
		// quite constrained in what you can do at such IRQL).
		if (PtrIoStackLocation->MinorFunction & IRP_MN_DPC) 
		{
			DebugTrace(DEBUG_TRACE_MISC,   " === Deferring Read ", 0 );
			CompleteIrp = FALSE;
			PostRequest = TRUE;
			try_return(RC = STATUS_PENDING);
		}

		PtrFileObject = PtrIoStackLocation->FileObject;
		ASSERT(PtrFileObject);

		// Get the FCB and CCB pointers
		Ext2GetFCB_CCB_VCB_FromFileObject ( 
			PtrFileObject, &PtrFCB, &PtrCCB, &PtrVCB );

		// Get some of the parameters supplied to us
		ByteOffset = PtrIoStackLocation->Parameters.Read.ByteOffset;
		ReadLength = PtrIoStackLocation->Parameters.Read.Length;

		CanWait = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);
		PagingIo = ((PtrIrp->Flags & IRP_PAGING_IO) ? TRUE : FALSE);
		NonBufferedIo = ((PtrIrp->Flags & IRP_NOCACHE) ? TRUE : FALSE);
		SynchronousIo = ((PtrFileObject->Flags & FO_SYNCHRONOUS_IO) ? TRUE : FALSE);

		if( PtrFCB && PtrFCB->FCBName && PtrFCB->FCBName->ObjectName.Length && PtrFCB->FCBName->ObjectName.Buffer )
		{
			DebugTrace(DEBUG_TRACE_FILE_NAME,   " === Read File Name : -%S-", PtrFCB->FCBName->ObjectName.Buffer );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_FILE_NAME,   " === Read File Name : -null-", 0);
		}
		
		DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->ByteCount           = 0x%8lx", PtrIoStackLocation->Parameters.Read.Length);
		DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->ByteOffset.LowPart  = 0x%8lx", PtrIoStackLocation->Parameters.Read.ByteOffset.LowPart);
		
		if( CanWait )
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->Can Wait ", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->Can't Wait ", 0 );
		}
		
		if( PagingIo )
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->Paging Io ", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->Not Paging Io", 0 );
		}

		if( SynchronousIo )
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->SynchronousIo ", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->ASynchronousIo ", 0 );
		}

		if( NonBufferedIo )
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->NonBufferedIo", 0 );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,   "  ->BufferedIo", 0 );
		}

	
		if (ReadLength == 0) 
		{
			// a 0 byte read can be immediately succeeded
			try_return();
		}

		// Is this a read of the volume itself ?
		if ( ( !PtrFCB && PtrVCB ) || PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB )
		{
			//
			//	>>>>>>>>>>>     Volume Read     <<<<<<<<<<<<
			//
			// Yep, we need to send this on to the disk driver after
			//	validation of the offset and length.

			//	PtrVCB = (PtrExt2VCB)(PtrFCB);

			if (PtrVCB->VCBFlags & EXT2_FCB_PAGE_FILE ) 
			{
				DebugTrace(DEBUG_TRACE_READ_DETAILS,  "[Read] *Volume Page File *", 0);
			}

			// Acquire the volume resource shared ...

			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Read]", PtrFileObject);
			}

			if( PagingIo )
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire VCB Shared [Read]", 0);
				DebugTraceState( "VCBPaging       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Read]", PtrVCB->PagingIoResource.ActiveCount, PtrVCB->PagingIoResource.NumberOfExclusiveWaiters, PtrVCB->PagingIoResource.NumberOfSharedWaiters );
				if (!ExAcquireResourceSharedLite(&(PtrVCB->PagingIoResource), FALSE )) 
				{
					// post the request to be processed in the context of a worker thread
					DebugTrace(DEBUG_TRACE_MISC,   "*** VCBPaging Acquisition FAILED [Read]", 0);
					CompleteIrp = FALSE;
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCBPaging Acquired [Read]", 0);
				
				PtrResourceAcquired = &(PtrVCB->PagingIoResource);
			}
			else
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire VCB Shared [Read]", 0);
				DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Read]", PtrVCB->VCBResource.ActiveCount, 
					PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
				if (!ExAcquireResourceSharedLite(&(PtrVCB->VCBResource), FALSE )) 
				{
					// post the request to be processed in the context of a worker thread
					DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Acquisition FAILED [Read]", 0);
					CompleteIrp = FALSE;
					PostRequest = TRUE;
					try_return(RC = STATUS_PENDING);
				}
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCB Acquired [Read]", 0);
				
				PtrResourceAcquired = &(PtrVCB->VCBResource);
			}
			if( !PagingIo )
			{
				if( PtrVCB->CommonVCBHeader.AllocationSize.QuadPart < ByteOffset.QuadPart )
				{
					RC = STATUS_END_OF_FILE;
					NumberBytesRead = 0;
					try_return();
				}
			}
			if( PagingIo || NonBufferedIo )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "[Volume Read] PagingIo or NonBufferedIo ", 0);
				CompleteIrp = FALSE;

				RC = Ext2PassDownSingleReadWriteIRP (
					PtrIrpContext, PtrIrp, PtrVCB, 
					ByteOffset, ReadLength, SynchronousIo);

				try_return();
			}
			else
			{
				//	Buffer Control Block
				PBCB		PtrBCB = NULL;
				DebugTrace(DEBUG_TRACE_READ_DETAILS,  "[Volume Read] BufferedIo ", 0);
				//
				//	Let the cache manager worry about this read...
				//	Pinned access should have been initiated.
				//	But checking anyway...
				//
				ASSERT( PtrVCB->PtrStreamFileObject );
				ASSERT( PtrVCB->PtrStreamFileObject->PrivateCacheMap );
				
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
					&ByteOffset,
					ReadLength,
					TRUE,
					&PtrBCB,
					&PtrPinnedReadBuffer) ) 
				{

					RC = STATUS_UNSUCCESSFUL;
					NumberBytesRead = 0;
					try_return();
				}
				else
				{
					PtrSystemBuffer = Ext2GetCallersBuffer(PtrIrp);
					RtlCopyBytes( PtrSystemBuffer, PtrPinnedReadBuffer, ReadLength );
					CcUnpinData( PtrBCB );
					PtrBCB = NULL;
					RC = STATUS_SUCCESS;
					NumberBytesRead = ReadLength;
					try_return();

				}
			}
		}


		// If the read request is directed to a page file 
		// send the request directly to the disk driver. 
		// For requests directed to a page file, you have to trust
		// that the offsets will be set correctly by the VMM. You should not
		// attempt to acquire any FSD resources either.

		if (PtrFCB->FCBFlags & EXT2_FCB_PAGE_FILE) 
		{
			IoMarkIrpPending(PtrIrp);
			// You will need to set a completion routine before invoking
			// a lower level driver
			//	forward request directly to disk driver
			// Ext2PageFileIo(PtrIrpContext, PtrIrp);
			DebugTrace( DEBUG_TRACE_SPECIAL, "[Read] To a *Page File* - Not handled \ngoing into a hang...", 0);
			CompleteIrp = FALSE;
			try_return(RC = STATUS_PENDING);
		}


		//
		//	If this read is directed to a directory...
		//	Paging IO is allowed though...
		//
		if ( ( PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY ) &&  !PagingIo )
		{
			RC = STATUS_INVALID_DEVICE_REQUEST;
			try_return();
		}

		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

		// Check whether the desired read can be allowed depending
		//	on any byte range locks that might exist. Note that for
		//	paging-io, no such checks should be performed.
		if (!PagingIo) 
		{
			//	Insert code to perform the check here ...
			//
			//	if (!Ext2CheckForByteLock(PtrFCB, PtrCCB, PtrIrp, PtrCurrentIoStackLocation)) 
			//	{
			//		try_return(RC = STATUS_FILE_LOCK_CONFLICT);
			//	}
		}

		// There are certain complications that arise when the same file stream
		// has been opened for cached and non-cached access. The FSD is then
		// responsible for maintaining a consistent view of the data seen by
		// the caller.
		// Also, it is possible for file streams to be mapped in both as data files
		// and as an executable. This could also lead to consistency problems since
		// there now exist two separate sections (and pages) containing file
		// information.
		// Read Chapter 10 for more information on the issues involved in
		// maintaining data consistency.
		// The test below flushes the data cached in system memory if the current
		// request madates non-cached access (file stream must be cached) and
		// (a) the current request is not paging-io which indicates it is not
		//		 a recursive I/O operation OR originating in the Cache Manager
		// (b) OR the current request is paging-io BUT it did not originate via
		//		 the Cache Manager (or is a recursive I/O operation) and we do
		//		 have an image section that has been initialized.



		#define	EXT2_REQ_NOT_VIA_CACHE_MGR(ptr)	(!MmIsRecursiveIoFault() && ((ptr)->ImageSectionObject != NULL))

		if( NonBufferedIo && (PtrReqdFCB->SectionObject.DataSectionObject != NULL) )
		{
			if	(!PagingIo || (EXT2_REQ_NOT_VIA_CACHE_MGR(&(PtrReqdFCB->SectionObject)))) 
			{
				CcFlushCache(&(PtrReqdFCB->SectionObject), &ByteOffset, ReadLength, &(PtrIrp->IoStatus));
				// If the flush failed, return error to the caller
				if (!NT_SUCCESS(RC = PtrIrp->IoStatus.Status)) 
				{
					try_return();
				}
			}
		}

		//
		//	Synchronizing with other reads and writes...
		//	Acquire the appropriate FCB resource shared
		//
		if ( PagingIo ) 
		{
			// Try to acquire the FCB PagingIoResource shared
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Read]", PtrFileObject);
			}
			DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCBPaging Shared [Read]", 0);
			DebugTraceState( "FCBPaging AC:0x%LX   SW:0x%LX   EX:0x%LX   [Read]", PtrReqdFCB->PagingIoResource.ActiveCount, PtrReqdFCB->PagingIoResource.NumberOfExclusiveWaiters, PtrReqdFCB->PagingIoResource.NumberOfSharedWaiters );
			if (!ExAcquireResourceSharedLite(&(PtrReqdFCB->PagingIoResource), CanWait)) 
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** FCBPaging Acquisition FAILED [Read]", 0);

				CompleteIrp = FALSE;
				PostRequest = TRUE;
				try_return(RC = STATUS_PENDING);
			}
			
			DebugTrace(DEBUG_TRACE_MISC,   "*** FCBPaging Acquired [Read]", 0);

			// Remember the resource that was acquired
			PtrResourceAcquired = &(PtrReqdFCB->PagingIoResource);
		} 
		else 
		{
			// Try to acquire the FCB MainResource shared
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Read]", PtrFileObject);
			}
			DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCB Shared [Read]", 0);
			DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [Read]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
			if (!ExAcquireResourceSharedLite(&(PtrReqdFCB->MainResource), CanWait)) 
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** FCB Acquisition FAILED [Read]", 0);
				CompleteIrp = FALSE;
				PostRequest = TRUE;
				try_return(RC = STATUS_PENDING);
			}
			DebugTrace(DEBUG_TRACE_MISC,   "*** FCB Acquired [Read]", 0);
			
			// Remember the resource that was acquired
			PtrResourceAcquired = &(PtrReqdFCB->MainResource);
		}

		//	Read in the File inode...
		Ext2InitializeFCBInodeInfo( PtrFCB );

		if (!PagingIo) 
		{
			LARGE_INTEGER  CurrentTime;
			KeQuerySystemTime( &CurrentTime );
			PtrFCB->LastAccessTime.QuadPart = CurrentTime.QuadPart;
		}

		// Validate start offset and length supplied.
		if (ByteOffset.QuadPart >= PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart )
		{
			// Starting offset is > file size
			try_return(RC = STATUS_END_OF_FILE);
		}

		/*
		 * Round down the size of Paging I/O requests too. I'm not
		 * sure if the FS driver is responsible for doing that, but
		 * all other drivers I have seen do it.
		 * -- Filip Navara, 18/08/2004
		 */
		if ( ByteOffset.QuadPart + ReadLength > PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart )
		{
			//	Read going beyond the end of file...
			//	Adjusting the Read Length...
			ReadLength = (UINT)(PtrReqdFCB->CommonFCBHeader.FileSize.QuadPart - ByteOffset.QuadPart);
			if (PagingIo)
				ReadLength = ROUND_TO_PAGES(ReadLength);
			ReadTruncated = TRUE;
		}

		// This is also a good place to set whether fast-io can be performed
		// on this particular file or not. Your FSD must make it's own
		// determination on whether or not to allow fast-io operations.
		// Commonly, fast-io is not allowed if any byte range locks exist
		// on the file or if oplocks prevent fast-io. Practically any reason
		// choosen by your FSD could result in your setting FastIoIsNotPossible
		// OR FastIoIsQuestionable instead of FastIoIsPossible.
		//
		// PtrReqdFCB->CommonFCBHeader.IsFastIoPossible = FastIoIsPossible;

		
		//	Branch here for cached vs non-cached I/O

		if (!NonBufferedIo) 
		{
			DebugTrace(DEBUG_TRACE_READ_DETAILS,  "[File Read] BufferedIo ", 0);

			// The caller wishes to perform cached I/O. Initiate caching if
			// this is the first cached I/O operation using this file object
			if (PtrFileObject->PrivateCacheMap == NULL) 
			{
				// This is the first cached I/O operation. You must ensure
				// that the FCB Common FCB Header contains valid sizes at this time
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
				CcMdlRead(PtrFileObject, &ByteOffset, TruncatedReadLength, &(PtrIrp->MdlAddress), &(PtrIrp->IoStatus));
				NumberBytesRead = PtrIrp->IoStatus.Information;
				RC = PtrIrp->IoStatus.Status;

				try_return();
			}

			// This is a regular run-of-the-mill cached I/O request. Let the
			// Cache Manager worry about it!
			// First though, we need a buffer pointer (address) that is valid
			PtrSystemBuffer = Ext2GetCallersBuffer(PtrIrp);
			ASSERT(PtrSystemBuffer);

			if (!CcCopyRead(PtrFileObject, &(ByteOffset), ReadLength, CanWait, PtrSystemBuffer, &(PtrIrp->IoStatus))) 
			{
				// The caller was not prepared to block and data is not immediately
				// available in the system cache
				DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure. Cannot read without blocking...", 0);
				CompleteIrp = FALSE;
				PostRequest = TRUE;
			
				// Mark Irp Pending ...
				IoMarkIrpPending( PtrIrp );
				RC = STATUS_PENDING;
				try_return();
			}

			// We have the data
			RC = PtrIrp->IoStatus.Status;
			NumberBytesRead = PtrIrp->IoStatus.Information;
			try_return();
		}
		else //	NonBuffered or Paged IO
		{
			LONGLONG	SingleIndirectBlockSize	;
			LONGLONG	DoubleIndirectBlockSize	;
			LONGLONG	TripleIndirectBlockSize	;
			LONGLONG	DirectBlockSize	;

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
			
			DebugTrace(DEBUG_TRACE_MISC,  "[File Read] Paging IO or NonBufferedIo ", 0);

			//	Calculating where the read should start from...
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

			/*
			if( ( ByteOffset.QuadPart + ReadLength ) > DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
			{
				//
				//	Handle Triple indirect blocks?
				//	A Pop up will do for now...
				//
				UNICODE_STRING ErrorMessage;
				Ext2CopyWideCharToUnicodeString( &ErrorMessage, L"Triple indirect blocks not supported as yet. - Ext2.sys" );
				DebugTrace(DEBUG_TRACE_ERROR,   "@@@@@@@@  Triple indirect blocks need to be read in! \n@@@@@@@@  This is not supported as yet!", 0);
				IoRaiseInformationalHardError(
						IO_ERR_DRIVER_ERROR,
						&ErrorMessage,
						KeGetCurrentThread( ) );

				Ext2DeallocateUnicodeString( &ErrorMessage );

				RC = STATUS_INSUFFICIENT_RESOURCES;
				try_return ( RC );

			}
			*/
			if( ( ByteOffset.QuadPart + ReadLength ) > DirectBlockSize &&
				( ByteOffset.QuadPart < DirectBlockSize + SingleIndirectBlockSize ) )
			{
				//
				//	Single Indirect Blocks required...
				//	Read in the single indirect blocks...
				//
				
				LARGE_INTEGER VolumeByteOffset;

				DebugTrace(DEBUG_TRACE_MISC,   "Reading in some Single Indirect Blocks", 0);

				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_IND_BLOCK ] * LogicalBlockSize;

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
					DebugTrace(DEBUG_TRACE_ASYNC,   "Cache read failiure while reading in volume meta data - Retrying", 0);
				}
			}
			if( ( ByteOffset.QuadPart + ReadLength ) > DirectBlockSize + SingleIndirectBlockSize &&
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

				if( ByteOffset.QuadPart + ReadLength >= 
					DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize)
				{
					EndIndirectBlock  = DoubleIndirectBlockSize;
				}
				else
				{
					EndIndirectBlock = ByteOffset.QuadPart + ReadLength - 
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
			
			if( ( ByteOffset.QuadPart + ReadLength ) > DirectBlockSize + SingleIndirectBlockSize + DoubleIndirectBlockSize )
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
				DebugTrace(DEBUG_TRACE_TRIPLE,   "ReadLength = 0x%lX", ReadLength );
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
				EndTIndirectBlock = ByteOffset.QuadPart + ReadLength;
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

				EndDIndirectBlock  = ByteOffset.QuadPart + ReadLength - 
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

						if( ByteOffset.QuadPart + ReadLength >= ByteOffsetTillHere + DoubleIndirectBlockSize)
						{
							EndIndirectBlock  = DoubleIndirectBlockSize;
						}
						else
						{
							EndIndirectBlock = ByteOffset.QuadPart + ReadLength - ByteOffsetTillHere;
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
			Index = ( (ReadLength - 2) / LogicalBlockSize + 2 );
			PtrIoRuns = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( Index * sizeof( EXT2_IO_RUN) )  );


			Start = (ULONG) ( ByteOffset.QuadPart - (LogicalBlockSize * LogicalBlockIndex) );
			BytesRemaining = (ULONG)( LogicalBlockSize * (LogicalBlockIndex +1) - ByteOffset.QuadPart );

			if( ReadLength > BytesRemaining )
			{
				End = Start + BytesRemaining;
			}
			else
			{
				End = Start + ReadLength;
			}
			BytesReadSoFar = 0;

			Index = 0;
			DebugTrace(DEBUG_TRACE_MISC, "\nDetermining the read IRPs that have to be passed down...", 0);
			
			while( 1 )
			{
				BytesReadSoFar += (End-Start);
				if( LogicalBlockIndex < NoOfDirectBlocks )
				{
					//	Direct Block
					PtrIoRuns[ Index ].LogicalBlock = PtrFCB->IBlock[ LogicalBlockIndex ];
				}
				else if( LogicalBlockIndex < (NoOfSingleIndirectBlocks + NoOfDirectBlocks) )
				{
					//	Single Indirect Block
					PtrIoRuns[ Index ].LogicalBlock = PtrPinnedSIndirectBlock[ LogicalBlockIndex - NoOfDirectBlocks ];
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
					//Ext2BreakPoint();
					Index--;
					break;

				}


				PtrIoRuns[ Index ].StartOffset = Start;
				PtrIoRuns[ Index ].EndOffset = End;
				PtrIoRuns[ Index ].PtrAssociatedIrp = NULL;

				DebugTrace( DEBUG_TRACE_MISC, "  Index = (%ld)", LogicalBlockIndex );
				DebugTrace( DEBUG_TRACE_MISC, "  Logical Block = (0x%lX)", PtrIoRuns[ Index ].LogicalBlock );
				DebugTrace( DEBUG_TRACE_MISC, "  Start = (0x%lX)", Start );
				DebugTrace( DEBUG_TRACE_MISC, "  End = (0x%lX)  ", End );
				DebugTrace( DEBUG_TRACE_MISC, "  Bytes read (0x%lX)", BytesReadSoFar );
				 
				

				if( BytesReadSoFar >= ReadLength )
					break;
				LogicalBlockIndex++;
				Start = 0;
				LeftOver = ReadLength - BytesReadSoFar;
				if( LeftOver > LogicalBlockSize )
					End = LogicalBlockSize;
				else
					End = LeftOver;
				//	Loop over to make the read request...
				Index++;
			}
			//
			//	Unpin the Indirect Blocks
			//
			if( PtrPinnedSIndirectBCB )
			{
				CcUnpinData( PtrPinnedSIndirectBCB  );
				PtrPinnedSIndirectBCB = NULL;
				PtrPinnedSIndirectBlock = NULL;
			}

			if( PtrPinnedDIndirectBCB )
			{
				CcUnpinData( PtrPinnedDIndirectBCB );
				PtrPinnedDIndirectBCB = NULL;
				PtrPinnedDIndirectBlock = NULL;
			}
			
			if( PtrPinnedTIndirectBCB)
			{
				CcUnpinData( PtrPinnedTIndirectBCB);
				PtrPinnedTIndirectBCB = NULL;
				PtrPinnedTIndirectBlock = NULL;
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
			if ( PtrTIArray )
			{
				ULONG	i;
				for( i = 0; i < TIArrayCount; i++ )
				{
					CcUnpinData( PtrTIArray->PtrBCB );
				}
				DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Read]", PtrTIArray );
				ExFreePool( PtrTIArray );
				PtrTIArray = NULL;
			}	
			//
			//	Pass down Associated IRPs to the Target Device Driver...
			//
			DebugTrace( DEBUG_TRACE_MISC, "Passing down the Read IRPs to the disk driver...", 0 );

			RC = Ext2PassDownMultiReadWriteIRP( PtrIoRuns, Index+1, ReadLength, PtrIrpContext, PtrFCB, SynchronousIo );
			
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
		except (EXCEPTION_EXECUTE_HANDLER) 
		{
			DebugTrace(DEBUG_TRACE_ERROR,   "@@@@@@@@  Exception Handler", 0);
		}
	}
	finally 
	{
		if ( PtrIoRuns )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Read]", PtrIoRuns );
			ExFreePool( PtrIoRuns );
		}
		//
		//	Unpin the Indirect Blocks
		//
		if( PtrPinnedSIndirectBCB )
		{
			CcUnpinData( PtrPinnedSIndirectBCB  );
			PtrPinnedSIndirectBCB = NULL;
		}

		if( PtrPinnedDIndirectBCB )
		{
			CcUnpinData( PtrPinnedDIndirectBCB );
			PtrPinnedDIndirectBCB = NULL;
		}
		if( PtrPinnedTIndirectBCB )
		{
			CcUnpinData( PtrPinnedTIndirectBCB );
			PtrPinnedTIndirectBCB = NULL;
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
		}

		if ( PtrTIArray )
		{
			ULONG	i;
			for( i = 0; i < TIArrayCount; i++ )
			{
				CcUnpinData( PtrTIArray->PtrBCB );
			}
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Read]", PtrTIArray );
			ExFreePool( PtrTIArray );
		}

		// Release any resources acquired here ...
		if (PtrResourceAcquired) 
		{
			Ext2ReleaseResource(PtrResourceAcquired);
			
			DebugTraceState( "Resource     AC:0x%LX   SW:0x%LX   EX:0x%LX   [Read]", 
				PtrResourceAcquired->ActiveCount, 
				PtrResourceAcquired->NumberOfExclusiveWaiters, 
				PtrResourceAcquired->NumberOfSharedWaiters );

			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Read]", PtrFileObject);
			}
			if( PtrVCB && PtrResourceAcquired == &(PtrVCB->VCBResource) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCB Released [Read]", 0);
			}
			else if( PtrVCB && PtrResourceAcquired == &(PtrVCB->PagingIoResource ) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCBPaging Released [Read]", 0);
			}
			else if( PtrReqdFCB && PtrResourceAcquired == &(PtrReqdFCB->PagingIoResource) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Paging Resource Released [Read]", 0);
			}
			else if(PtrReqdFCB && PtrResourceAcquired == &(PtrReqdFCB->MainResource) )
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Resource Released [Read]", 0);
			}
			else
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** Unknown Resource Released [Read]", 0);
			}

			PtrResourceAcquired = NULL;
		}

		// Post IRP if required
		if ( PostRequest ) 
		{

			// Implement a routine that will queue up the request to be executed
			// later (asynchronously) in the context of a system worker thread.
			// See Chapter 10 for details.

			// Lock the callers buffer here. Then invoke a common routine to
			// perform the post operation.
			if (!(PtrIoStackLocation->MinorFunction & IRP_MN_MDL)) 
			{
				RC = Ext2LockCallersBuffer(PtrIrp, TRUE, ReadLength);
				ASSERT(NT_SUCCESS(RC));
			}

			// Perform the post operation which will mark the IRP pending
			// and will return STATUS_PENDING back to us
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
		}
		else if (CompleteIrp && !(RC == STATUS_PENDING)) 
		{
			// For synchronous I/O, the FSD must maintain the current byte offset
			// Do not do this however, if I/O is marked as paging-io
			if (SynchronousIo && !PagingIo && NT_SUCCESS(RC)) 
			{
				PtrFileObject->CurrentByteOffset = RtlLargeIntegerAdd(ByteOffset, RtlConvertUlongToLargeInteger((unsigned long)NumberBytesRead));
			}

			// If the read completed successfully and this was not a paging-io
			// operation, set a flag in the CCB that indicates that a read was
			// performed and that the file time should be updated at cleanup
			if (NT_SUCCESS(RC) && !PagingIo) 
			{
				Ext2SetFlag(PtrCCB->CCBFlags, EXT2_CCB_ACCESSED);
			}

			// Can complete the IRP here if no exception was encountered
			if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_EXCEPTION)) 
			{
				
				PtrIrp->IoStatus.Status = RC;
				PtrIrp->IoStatus.Information = NumberBytesRead;

				// Free up the Irp Context
				Ext2ReleaseIrpContext(PtrIrpContext);
	
				// complete the IRP
				IoCompleteRequest(PtrIrp, IO_DISK_INCREMENT);
			}
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
* Function: Ext2GetCallersBuffer()
*
* Description:
*	Obtain a pointer to the caller's buffer.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
void * NTAPI Ext2GetCallersBuffer (
		PIRP  PtrIrp )
{
	void	* ReturnedBuffer = NULL;

	// If an MDL is supplied, use it.
	if (PtrIrp->MdlAddress) 
	{
		ReturnedBuffer = MmGetSystemAddressForMdl(PtrIrp->MdlAddress);
	} 
	else 
	{
		ReturnedBuffer = PtrIrp->UserBuffer;
	}

	return (ReturnedBuffer);
}



/*************************************************************************
*
* Function: Ext2LockCallersBuffer()
*
* Description:
*	Obtain a MDL that describes the buffer. Lock pages for I/O
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2LockCallersBuffer(
PIRP				PtrIrp,
BOOLEAN			IsReadOperation,
uint32			Length)
{
	NTSTATUS			RC = STATUS_SUCCESS;
	PMDL				PtrMdl = NULL;

	ASSERT(PtrIrp);
	
	try 
	{
		// Is a MDL already present in the IRP
		if( !(PtrIrp->MdlAddress) )
		{
			// Allocate a MDL
			if (!(PtrMdl = IoAllocateMdl(PtrIrp->UserBuffer, Length, FALSE, FALSE, PtrIrp))) 
			{
				RC = STATUS_INSUFFICIENT_RESOURCES;
				try_return();
			}

			// Probe and lock the pages described by the MDL
			// We could encounter an exception doing so, swallow the exception
			// NOTE: The exception could be due to an unexpected (from our
			// perspective), invalidation of the virtual addresses that comprise
			// the passed in buffer
			try 
			{
				MmProbeAndLockPages(PtrMdl, PtrIrp->RequestorMode, (IsReadOperation ? IoWriteAccess:IoReadAccess));
			} 
			except(EXCEPTION_EXECUTE_HANDLER) 
			{
				RC = STATUS_INVALID_USER_BUFFER;
			}
		}

		try_exit:	NOTHING;

	} 
	finally 
	{
		if (!NT_SUCCESS(RC) && PtrMdl) 
		{
			IoFreeMdl(PtrMdl);
			// You MUST NULL out the MdlAddress field in the IRP after freeing
			// the MDL, else the I/O Manager will also attempt to free the MDL
			// pointed to by that field during I/O completion. Obviously, the
			// pointer becomes invalid once you free the allocated MDL and hence
			// you will encounter a system crash during IRP completion.
			PtrIrp->MdlAddress = NULL;
		}
	}

	return(RC);
}



/*************************************************************************
*
* Function: Ext2MdlComplete()
*
* Description:
*	Tell Cache Manager to release MDL (and possibly flush).
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None.
*
*************************************************************************/
void NTAPI Ext2MdlComplete(
	PtrExt2IrpContext			PtrIrpContext,
	PIRP						PtrIrp,
	PIO_STACK_LOCATION			PtrIoStackLocation,
	BOOLEAN						ReadCompletion)
{

	NTSTATUS					RC = STATUS_SUCCESS;
	PFILE_OBJECT			PtrFileObject = NULL;

	PtrFileObject = PtrIoStackLocation->FileObject;
	ASSERT(PtrFileObject);

	// Not much to do here.
	if( ReadCompletion ) 
	{
		CcMdlReadComplete( PtrFileObject, PtrIrp->MdlAddress );
	} 
	else 
	{
		// The Cache Manager needs the byte offset in the I/O stack location.
		CcMdlWriteComplete( PtrFileObject, &(PtrIoStackLocation->Parameters.Write.ByteOffset), PtrIrp->MdlAddress );
	}

	// Clear the MDL address field in the IRP so the IoCompleteRequest()
	// does not try to play around with the MDL.
	PtrIrp->MdlAddress = NULL;

	// Free up the Irp Context.
	Ext2ReleaseIrpContext(PtrIrpContext);

	// Complete the IRP.
	PtrIrp->IoStatus.Status = RC;
	PtrIrp->IoStatus.Information = 0;
	IoCompleteRequest(PtrIrp, IO_NO_INCREMENT);

	return;
}

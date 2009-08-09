/*************************************************************************
*
* File: fastio.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the various "fast-io" calls.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_FAST_IO



/*************************************************************************
*
* Function: Ext2FastIoCheckIfPossible()
*
* Description:
*	To fast-io or not to fast-io, that is the question ...
*	This routine helps the I/O Manager determine whether the FSD wishes
*	to permit fast-io on a specific file stream.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoCheckIfPossible(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN BOOLEAN						Wait,
IN ULONG							LockKey,
IN BOOLEAN						CheckForReadOperation,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;
	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	LARGE_INTEGER		IoLength;

	// Obtain a pointer to the FCB and CCB for the file stream.
	PtrCCB = (PtrExt2CCB)(FileObject->FsContext2);
	ASSERT(PtrCCB);
	PtrFCB = PtrCCB->PtrFCB;
	ASSERT(PtrFCB);

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoCheckIfPossible - Denying", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	
	AssertFCBorVCB( PtrFCB );

/*	if( !( PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB 
			|| PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB ) )
	{
		//	Ext2BreakPoint();
		DebugTrace(DEBUG_TRACE_ERROR,   "~~~[FastIO call]~~~  Invalid FCB...", 0);

		return FALSE;
	}
*/

	return FALSE;

	// Validate that this is a fast-IO request to a regular file.
	// The sample FSD for example, will not allow fast-IO requests
	// to volume objects, or to directories.
	if ((PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) ||
		 (PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY)) 
	{
		// This is not allowed.
		return(ReturnedStatus);
	}

	IoLength = RtlConvertUlongToLargeInteger(Length);
	
	// Your FSD can determine the checks that it needs to perform.
	// Typically, a FSD will check whether there exist any byte-range
	// locks that would prevent a fast-IO operation from proceeding.
	
	// ... (FSD specific checks go here).
	
	if (CheckForReadOperation) 
	{
		// Chapter 11 describes how to use the FSRTL package for byte-range
		// lock requests. The following routine is exported by the FSRTL
		// package and it returns TRUE if the read operation should be
		// allowed to proceed based on the status of the current byte-range
		// locks on the file stream. If you do not use the FSRTL package
		// for byte-range locking support, then you must substitute your
		// own checks over here.
		// ReturnedStatus = FsRtlFastCheckLockForRead(&(PtrFCB->FCBByteRangeLock),
		//							FileOffset, &IoLength, LockKey, FileObject,
      //                     PsGetCurrentProcess());
	} 
	else 
	{
		// This is a write request. Invoke the FSRTL byte-range lock package
		// to see whether the write should be allowed to proceed.
		// ReturnedStatus = FsRtlFastCheckLockForWrite(&(PtrFCB->FCBByteRangeLock),
		//							FileOffset, &IoLength, LockKey, FileObject,
      //                     PsGetCurrentProcess());
	}
	
	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoRead()
*
* Description:
*	Bypass the traditional IRP method to perform a read operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoRead(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN BOOLEAN						Wait,
IN ULONG							LockKey,
OUT PVOID						Buffer,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{

	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoRead", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	

	try 
	{

		try 
		{

			// Chapter 11 describes how to roll your own fast-IO entry points.
			// Typically, you will acquire appropriate resources here and
			// then (maybe) forward the request to FsRtlCopyRead().
			// If you are a suitably complex file system, you may even choose
			// to do some pre-processing (e.g. prefetching data from someplace)
			// before passing on the request to the FSRTL package.

			// Of course, you also have the option of bypassing the FSRTL
			// package completely and simply forwarding the request directly
			// to the NT Cache Manager.

			// Bottom line is that you have complete flexibility on determining
			// what you decide to do here. Read Chapter 11 well (and obviously
			// other related issues) before filling in this and other fast-IO
			// dispatch entry points.

			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
	
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
	
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);

		}
	} finally {

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoWrite()
*
* Description:
*	Bypass the traditional IRP method to perform a write operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoWrite(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN BOOLEAN						Wait,
IN ULONG							LockKey,
OUT PVOID						Buffer,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoWrite", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	try 
	{
		try 
		{

			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
	
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
	
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);

		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoQueryBasicInfo()
*
* Description:
*	Bypass the traditional IRP method to perform a query basic
*	information operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoQueryBasicInfo(
IN PFILE_OBJECT					FileObject,
IN BOOLEAN							Wait,
OUT PFILE_BASIC_INFORMATION	Buffer,
OUT PIO_STATUS_BLOCK 			IoStatus,
IN PDEVICE_OBJECT					DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS			RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoQueryBasicInfo", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	try
	{

		try
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		}
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}

/*************************************************************************
*
* Function: Ext2FastIoQueryStdInfo()
*
* Description:
*	Bypass the traditional IRP method to perform a query standard
*	information operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoQueryStdInfo(
IN PFILE_OBJECT						FileObject,
IN BOOLEAN							Wait,
OUT PFILE_STANDARD_INFORMATION 		Buffer,
OUT PIO_STATUS_BLOCK 				IoStatus,
IN PDEVICE_OBJECT					DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoQueryStdInfo", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	try 
	{

		try
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}

/*************************************************************************
*
* Function: Ext2FastIoLock()
*
* Description:
*	Bypass the traditional IRP method to perform a byte range lock
*	operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoLock(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN PLARGE_INTEGER				Length,
PEPROCESS						ProcessId,
ULONG								Key,
BOOLEAN							FailImmediately,
BOOLEAN							ExclusiveLock,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS			RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoLock", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	
	try 
	{

		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		}
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	} 
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoUnlockSingle()
*
* Description:
*	Bypass the traditional IRP method to perform a byte range unlock
*	operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoUnlockSingle(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN PLARGE_INTEGER				Length,
PEPROCESS						ProcessId,
ULONG								Key,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoUnlockSingle", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try 
	{

		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	} 
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoUnlockAll()
*
* Description:
*	Bypass the traditional IRP method to perform multiple byte range unlock
*	operations.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoUnlockAll(
IN PFILE_OBJECT				FileObject,
PEPROCESS						ProcessId,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoUnlockAll", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try
	{
		try
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoUnlockAllByKey()
*
* Description:
*	Bypass the traditional IRP method to perform multiple byte range unlock
*	operations.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoUnlockAllByKey(
IN PFILE_OBJECT				FileObject,
PVOID						ProcessId,
ULONG								Key,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS			RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoUnlockAllByKey", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}
	try 
	{

		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
		}
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoAcqCreateSec()
*
* Description:
*	Not really a fast-io operation. Used by the VMM to acquire FSD resources
*	before processing a file map (create section object) request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None (we must be prepared to handle VMM initiated calls)
*
*************************************************************************/
void NTAPI Ext2FastIoAcqCreateSec(
IN PFILE_OBJECT			FileObject)
{
	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;

	// Obtain a pointer to the FCB and CCB for the file stream.
	PtrCCB = (PtrExt2CCB)(FileObject->FsContext2);
	ASSERT(PtrCCB);
	PtrFCB = PtrCCB->PtrFCB;
	ASSERT(PtrFCB);

	AssertFCB( PtrFCB );

/*	if( PtrFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB )
	{
		//	Ext2BreakPoint();
		DebugTrace(DEBUG_TRACE_ERROR,   "~~~[FastIO call]~~~  Invalid FCB...", 0);
		return;
	}	*/
	
	PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~ Ext2FastIoAcqCreateSec", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	// Acquire the MainResource exclusively for the file stream
	
	DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire FCB Exclusively [FastIo]", 0);

	DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
	ExAcquireResourceExclusiveLite(&(PtrReqdFCB->MainResource), TRUE);

	DebugTrace(DEBUG_TRACE_MISC,"*** FCB acquired [FastIo]", 0);

	// Although this is typically not required, the sample FSD will
	// also acquire the PagingIoResource exclusively at this time
	// to conform with the resource acquisition described in the set
	// file information routine. Once again though, you will probably
	// not need to do this.
	DebugTrace(DEBUG_TRACE_MISC,"*** Attempting to acquire FCBPaging Exclusively [FastIo]", 0);
	DebugTraceState( "FCBPaging AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", PtrReqdFCB->PagingIoResource.ActiveCount, PtrReqdFCB->PagingIoResource.NumberOfExclusiveWaiters, PtrReqdFCB->PagingIoResource.NumberOfSharedWaiters );
	ExAcquireResourceExclusiveLite(&(PtrReqdFCB->PagingIoResource), TRUE);
	
	DebugTrace(DEBUG_TRACE_MISC,"*** FCBPaging acquired [FastIo]", 0);

	return;
}


/*************************************************************************
*
* Function: Ext2FastIoRelCreateSec()
*
* Description:
*	Not really a fast-io operation. Used by the VMM to release FSD resources
*	after processing a file map (create section object) request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2FastIoRelCreateSec(
IN PFILE_OBJECT			FileObject)
{

	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;

	// Obtain a pointer to the FCB and CCB for the file stream.
	PtrCCB = (PtrExt2CCB)(FileObject->FsContext2);
	ASSERT(PtrCCB);
	PtrFCB = PtrCCB->PtrFCB;
	ASSERT(PtrFCB);
	AssertFCB( PtrFCB );

/*	if( PtrFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB )
	{
		//	Ext2BreakPoint();
		DebugTrace(DEBUG_TRACE_ERROR,   "~~~[FastIO call]~~~  Invalid FCB...", 0);
		return;
	}*/

	PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoRelCreateSec", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	// Release the PagingIoResource for the file stream
	Ext2ReleaseResource(&(PtrReqdFCB->PagingIoResource));
	DebugTrace(DEBUG_TRACE_MISC, "*** FCBPaging Released in [FastIo]", 0);
	DebugTraceState( "FCBPaging AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", 
		PtrReqdFCB->PagingIoResource.ActiveCount, 
		PtrReqdFCB->PagingIoResource.NumberOfExclusiveWaiters, 
		PtrReqdFCB->PagingIoResource.NumberOfSharedWaiters );

	// Release the MainResource for the file stream
	Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
	DebugTrace(DEBUG_TRACE_MISC, "*** FCB Released [FastIo]", 0);
	DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", 
		PtrReqdFCB->MainResource.ActiveCount, 
		PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
		PtrReqdFCB->MainResource.NumberOfSharedWaiters );

	return;
}


/*************************************************************************
*
* Function: Ext2AcqLazyWrite()
*
* Description:
*	Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*	resources before performing a delayed write (write behind/lazy write)
*	operation.
*	NOTE: this function really must succeed since the Cache Manager will
*			typically ignore failure and continue on ...
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE (Cache Manager does not tolerate FALSE well)
*
*************************************************************************/
BOOLEAN NTAPI Ext2AcqLazyWrite(
IN PVOID						Context,
IN BOOLEAN						Wait)
{
	BOOLEAN				ReturnedStatus = TRUE;
	
	PtrExt2VCB			PtrVCB = NULL;
	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;

	// The context is whatever we passed to the Cache Manager when invoking
	// the CcInitializeCacheMaps() function. In the case of the sample FSD
	// implementation, this context is a pointer to the CCB structure.

	ASSERT(Context);
	PtrCCB = (PtrExt2CCB)(Context);
	
	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2AcqLazyWrite", 0);

	if(PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_CCB)
	{
		//
		//	Acquiring Resource for a file write...
		//
		PtrFCB = PtrCCB->PtrFCB;
		AssertFCB( PtrFCB );
	}
	else if( PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB )
	{
		//
		//	Acquiring Resource for a volume write...
		//
		PtrVCB = ( PtrExt2VCB )PtrCCB;
		PtrCCB = NULL;
		DebugTrace(DEBUG_TRACE_MISC,"~~~[FastIO call]~~~  Ext2AcqLazyWrite - for Volume", 0);
		
		//	Acquire nothing...
		//	Just proceed...
		return TRUE;

	}
	else if( PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB ) 
	{
		//
		//	This must have been a FCB created / maintained on the FSD's initiative...
		//	This would have been done to cache access to a directory...
		//
		PtrFCB = ( PtrExt2FCB )PtrCCB;
		PtrCCB = NULL;
	}
	else
	{
		DebugTrace(DEBUG_TRACE_ERROR, "~~~[FastIO call]~~~  Ext2AcqLazyWrite - Invalid context", 0);
		Ext2BreakPoint();
		return FALSE;
	}
	
	if( PtrCCB && PtrCCB->PtrFileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", PtrCCB->PtrFileObject );
	}

	AssertFCB( PtrFCB );


	PtrReqdFCB = &(PtrFCB->NTRequiredFCB);


	// Acquire the MainResource in the FCB exclusively. Then, set the
	// lazy-writer thread id in the FCB structure for identification when
	// an actual write request is received by the FSD.
	// Note: The lazy-writer typically always supplies WAIT set to TRUE.
	
	DebugTrace(DEBUG_TRACE_MISC,"*** Attempting to acquire FCB Exclusively [FastIo]", 0);

	DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
	if (!ExAcquireResourceExclusiveLite(&(PtrReqdFCB->MainResource),
													  Wait)) 
	{
		DebugTrace(DEBUG_TRACE_MISC,"*** Attempt to acquire FCB FAILED [FastIo]", 0);
		ReturnedStatus = FALSE;
	} 
	else
	{
		DebugTrace(DEBUG_TRACE_MISC,"*** FCB acquired [FastIo]", 0);
		// Now, set the lazy-writer thread id.
		ASSERT(!(PtrFCB->LazyWriterThreadID));
		PtrFCB->LazyWriterThreadID = (unsigned int)(PsGetCurrentThread());
	}

	// If your FSD needs to perform some special preparations in anticipation
	// of receving a lazy-writer request, do so now.

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2RelLazyWrite()
*
* Description:
*	Not really a fast-io operation. Used by the NT Cache Mgr to release FSD
*	resources after performing a delayed write (write behind/lazy write)
*	operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2RelLazyWrite(
IN PVOID							Context)
{

	PtrExt2VCB			PtrVCB = NULL;
	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;

	// The context is whatever we passed to the Cache Manager when invoking
	// the CcInitializeCacheMaps() function. In the case of the sample FSD
	// implementation, this context is a pointer to the CCB structure.

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2RelLazyWrite", 0);

	ASSERT(Context);
	PtrCCB = (PtrExt2CCB)(Context);
	
	if(PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_CCB)
	{
		PtrFCB = PtrCCB->PtrFCB;
		AssertFCB( PtrFCB );
	}
	else if( PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB )
	{
		PtrVCB = ( PtrExt2VCB )PtrCCB;
		PtrCCB = NULL;
		DebugTrace(DEBUG_TRACE_MISC,"~~~[FastIO call]~~~  Ext2RelLazyWrite - for Volume", 0);
		
		//	Acquire was acquired nothing...
		//	Just return...
		return;

	}
	else if( PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB ) 
	{
		//
		//	This must have been a FCB created / maintained on the FSD's initiative...
		//	This would have been done to cache access to a directory...
		//
		PtrFCB = ( PtrExt2FCB )PtrCCB;
		PtrCCB = NULL;
	}
	else
	{
		DebugTrace(DEBUG_TRACE_ERROR, "~~~[FastIO call]~~~  Ext2RelLazyWrite - Invalid context", 0);
		Ext2BreakPoint();
		return ;
	}

	if( PtrCCB && PtrCCB->PtrFileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", PtrCCB->PtrFileObject );
	}

	PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

	// Remove the current thread-id from the FCB and release the MainResource.
	ASSERT( (PtrFCB->LazyWriterThreadID) == (unsigned int)PsGetCurrentThread() );
	PtrFCB->LazyWriterThreadID = 0;


	// Release the acquired resource.
	Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
	DebugTrace(DEBUG_TRACE_MISC, "*** FCB Released [FastIo]", 0);
	DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", 
		PtrReqdFCB->MainResource.ActiveCount, 
		PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
		PtrReqdFCB->MainResource.NumberOfSharedWaiters );

	//
	// Undo whatever else seems appropriate at this time...
	//

	return;
}


/*************************************************************************
*
* Function: Ext2AcqReadAhead()
*
* Description:
*	Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*	resources before performing a read-ahead operation.
*	NOTE: this function really must succeed since the Cache Manager will
*			typically ignore failure and continue on ...
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE (Cache Manager does not tolerate FALSE well)
*
*************************************************************************/
BOOLEAN NTAPI Ext2AcqReadAhead(
IN PVOID						Context,
IN BOOLEAN						Wait)
{

	BOOLEAN				ReturnedStatus = TRUE;

	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;

	// The context is whatever we passed to the Cache Manager when invoking
	// the CcInitializeCacheMaps() function. In the case of the sample FSD
	// implementation, this context is a pointer to the CCB structure.
	
	ASSERT(Context);
	PtrCCB = (PtrExt2CCB)(Context);
	ASSERT(PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_CCB);

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2AcqReadAhead", 0);
	if( PtrCCB && PtrCCB->PtrFileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", PtrCCB->PtrFileObject );
	}

	PtrFCB = PtrCCB->PtrFCB;
	ASSERT(PtrFCB);

	AssertFCB( PtrFCB );
/*
	if( PtrFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB )
	{
		//	Ext2BreakPoint();
		DebugTrace(DEBUG_TRACE_ERROR,   "~~~[FastIO call]~~~  Invalid FCB...", 0);
		return TRUE;
	}	*/

	PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

	// Acquire the MainResource in the FCB shared.
	// Note: The read-ahead thread typically always supplies WAIT set to TRUE.
	DebugTrace(DEBUG_TRACE_MISC,"*** Attempting to acquire FCB Shared [FastIo]", 0);

	DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
	if (!ExAcquireResourceSharedLite(&(PtrReqdFCB->MainResource), Wait)) 
	{
		DebugTrace(DEBUG_TRACE_MISC,"*** Attempt to acquire FCB FAILED [FastIo]", 0);
		ReturnedStatus = FALSE;
	}
	else
	{
		DebugTrace(DEBUG_TRACE_MISC,"***  FCB acquired [FastIo]", 0);
	}

	// If your FSD needs to perform some special preparations in anticipation
	// of receving a read-ahead request, do so now.

	return ReturnedStatus;
	
}



/*************************************************************************
*
* Function: Ext2RelReadAhead()
*
* Description:
*	Not really a fast-io operation. Used by the NT Cache Mgr to release FSD
*	resources after performing a read-ahead operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2RelReadAhead(
IN PVOID							Context)
{
	PtrExt2FCB			PtrFCB = NULL;
	PtrExt2CCB			PtrCCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;


	// The context is whatever we passed to the Cache Manager when invoking
	// the CcInitializeCacheMaps() function. In the case of the sample FSD
	// implementation, this context is a pointer to the CCB structure.

	ASSERT(Context);
	PtrCCB = (PtrExt2CCB)(Context);
	ASSERT(PtrCCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_CCB);

	PtrFCB = PtrCCB->PtrFCB;
	
	AssertFCB( PtrFCB );
	
	//	ASSERT(PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB );

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2RelReadAhead", 0);
	if( PtrCCB && PtrCCB->PtrFileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", PtrCCB->PtrFileObject );
	}

/*	if( PtrFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB )
	{
		//	Ext2BreakPoint();
		DebugTrace(DEBUG_TRACE_ERROR,   "~~~[FastIO call]~~~  Invalid FCB...", 0);
		return;
	}	*/

	PtrReqdFCB = &(PtrFCB->NTRequiredFCB);


	// Release the acquired resource.
	Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
	DebugTrace(DEBUG_TRACE_MISC, "*** FCB Released [FastIo]", 0);
	DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [FastIo]", 
		PtrReqdFCB->MainResource.ActiveCount, 
		PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
		PtrReqdFCB->MainResource.NumberOfSharedWaiters );

	// Of course, your FSD should undo whatever else seems appropriate at this
	// time.

	return;
}


/* the remaining are only valid under NT Version 4.0 and later */
#if(_WIN32_WINNT >= 0x0400)


/*************************************************************************
*
* Function: Ext2FastIoQueryNetInfo()
*
* Description:
*	Get information requested by a redirector across the network. This call
*	will originate from the LAN Manager server.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoQueryNetInfo(
IN PFILE_OBJECT									FileObject,
IN BOOLEAN											Wait,
OUT PFILE_NETWORK_OPEN_INFORMATION 			Buffer,
OUT PIO_STATUS_BLOCK 							IoStatus,
IN PDEVICE_OBJECT									DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();
	
	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoQueryNetInfo", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try 
	{

		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	} 
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoMdlRead()
*
* Description:
*	Bypass the traditional IRP method to perform a MDL read operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoMdlRead(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN ULONG							LockKey,
OUT PMDL							*MdlChain,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoMdlRead", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try 
	{

		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
	
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	} 
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoMdlReadComplete()
*
* Description:
*	Bypass the traditional IRP method to inform the NT Cache Manager and the
*	FSD that the caller no longer requires the data locked in the system cache
*	or the MDL to stay around anymore ..
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoMdlReadComplete(
IN PFILE_OBJECT				FileObject,
OUT PMDL							MdlChain,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoMdlReadComplete", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try 
	{

		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
		
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
	
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
	
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);

		}
	} 
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoPrepareMdlWrite()
*
* Description:
*	Bypass the traditional IRP method to prepare for a MDL write operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoPrepareMdlWrite(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
IN ULONG							Length,
IN ULONG							LockKey,
OUT PMDL							*MdlChain,
OUT PIO_STATUS_BLOCK			IoStatus,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoPrepareMdlWrite", 0);
	if( FileObject )
	{
		DebugTrace(  DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try 
	{
		try 
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
		
		} except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) {
	
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
	
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);

		}
	} 
	finally 
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoMdlWriteComplete()
*
* Description:
*	Bypass the traditional IRP method to inform the NT Cache Manager and the
*	FSD that the caller has updated the contents of the MDL. This data can
*	now be asynchronously written out to secondary storage by the Cache Mgr.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN NTAPI Ext2FastIoMdlWriteComplete(
IN PFILE_OBJECT				FileObject,
IN PLARGE_INTEGER				FileOffset,
OUT PMDL							MdlChain,
IN PDEVICE_OBJECT				DeviceObject)
{
	BOOLEAN				ReturnedStatus = FALSE;		// fast i/o failed/not allowed
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoMdlWriteComplete", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try
	{

		try
		{
	
			// See description in Ext2FastIoRead() before filling-in the
			// stub here.
			NOTHING;
		
		}
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(ReturnedStatus);
}


/*************************************************************************
*
* Function: Ext2FastIoAcqModWrite()
*
* Description:
*	Not really a fast-io operation. Used by the VMM to acquire FSD resources
*	before initiating a write operation via the Modified Page/Block Writer.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (try not to return an error, will 'ya ? :-)
*
*************************************************************************/
NTSTATUS NTAPI Ext2FastIoAcqModWrite(
IN PFILE_OBJECT					FileObject,
IN PLARGE_INTEGER					EndingOffset,
OUT PERESOURCE						*ResourceToRelease,
IN PDEVICE_OBJECT					DeviceObject)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,"~~~[FastIO call]~~~  Ext2FastIoAcqModWrite", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try
	{
		try
		{

			// You must determine which resource(s) you would like to
			// acquire at this time. You know that a write is imminent;
			// you will probably therefore acquire appropriate resources
			// exclusively.

			// You must first get the FCB and CCB pointers from the file object
			// that is passed in to this function (as an argument). Note that
			// the ending offset (when examined in conjunction with current valid data
			// length) may help you in determining the appropriate resource(s) to acquire.

			// For example, if the ending offset is beyond current valid data length,
			// you may decide to acquire *both* the MainResource and the PagingIoResource
			// exclusively; otherwise, you may decide simply to acquire the PagingIoResource.

			// Consult the text for more information on synchronization in FSDs.

			// One final note; the VMM expects that you will return a pointer to
			// the resource that you acquired (single return value). This pointer
			// will be returned back to you in the release call (below).

			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(RC);
}


/*************************************************************************
*
* Function: Ext2FastIoRelModWrite()
*
* Description:
*	Not really a fast-io operation. Used by the VMM to release FSD resources
*	after processing a modified page/block write operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (an error returned here is really not expected!)
*
*************************************************************************/
NTSTATUS NTAPI Ext2FastIoRelModWrite(
IN PFILE_OBJECT				FileObject,
IN PERESOURCE					ResourceToRelease,
IN PDEVICE_OBJECT				DeviceObject)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

   	DebugTrace( DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoRelModWrite", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try
	{
		try
		{

			// The MPW has complete the write for modified pages and therefore
			// wants you to release pre-acquired resource(s).

			// You must undo here whatever it is that you did in the
			// Ext2FastIoAcqModWrite() call above.

			NOTHING;
	
		}
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(RC);
}


/*************************************************************************
*
* Function: Ext2FastIoAcqCcFlush()
*
* Description:
*	Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*	resources before performing a CcFlush() operation on a specific file
*	stream.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2FastIoAcqCcFlush(
IN PFILE_OBJECT			FileObject,
IN PDEVICE_OBJECT			DeviceObject)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoAcqCcFlush", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try
	{
		try
		{
			// Acquire appropriate resources that will allow correct synchronization
			// with a flush call (and avoid deadlock).
			NOTHING;
	
		}
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(RC);
}


/*************************************************************************
*
* Function: Ext2FastIoRelCcFlush()
*
* Description:
*	Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*	resources before performing a CcFlush() operation on a specific file
*	stream.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2FastIoRelCcFlush(
IN PFILE_OBJECT			FileObject,
IN PDEVICE_OBJECT			DeviceObject)
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;

	FsRtlEnterFileSystem();

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "~~~[FastIO call]~~~  Ext2FastIoRelCcFlush", 0);
	if( FileObject )
	{
		DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [FastIO]", FileObject);
	}

	try
	{
		try
		{
			// Release resources acquired in Ext2FastIoAcqCcFlush() above.
			NOTHING;
	
		} 
		except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
		{
			RC = Ext2ExceptionHandler(PtrIrpContext, NULL);
			Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
		}
	}
	finally
	{

	}
	
	FsRtlExitFileSystem();

	return(RC);
}

#endif	//_WIN32_WINNT >= 0x0400

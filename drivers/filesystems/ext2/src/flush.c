/*************************************************************************
*
* File: flush.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "Flush Buffers" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_FLUSH

#define			DEBUG_LEVEL						(DEBUG_TRACE_FLUSH)


/*************************************************************************
*
* Function: Ext2Flush()
*
* Description:
*	The I/O Manager will invoke this routine to handle a flush buffers
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
NTSTATUS NTAPI Ext2Flush(
	PDEVICE_OBJECT		DeviceObject,		//	the logical volume device object
	PIRP				Irp)				//	I/O Request Packet
{
	NTSTATUS			RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "Flush IRP Received...", 0);
	
	// Ext2BreakPoint();

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

		RC = Ext2CommonFlush(PtrIrpContext, Irp);

	} except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) {

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
* Function: Ext2CommonFlush()
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
NTSTATUS NTAPI Ext2CommonFlush(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp)
{
	NTSTATUS					RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION	PtrIoStackLocation = NULL;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;
	PtrExt2VCB				PtrVCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	BOOLEAN					AcquiredFCB = FALSE;
	BOOLEAN					PostRequest = FALSE;
	BOOLEAN					CanWait = TRUE;

	try {
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

		/*ASSERT(PtrFCB);
		ASSERT(PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB );*/

		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

		// Get some of the parameters supplied to us
		CanWait = ((PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);

		// If we cannot wait, post the request immediately since a flush is inherently blocking/synchronous.
		if (!CanWait) {
			PostRequest = TRUE;
			try_return();
		}

		// Check the type of object passed-in. That will determine the course of
		// action we take.
		if ((PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) || (PtrFCB->FCBFlags & EXT2_FCB_ROOT_DIRECTORY)) {

			if (PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB) {
				PtrVCB = (PtrExt2VCB)(PtrFCB);
			} else {
				PtrVCB = PtrFCB->PtrVCB;
			}

			// The caller wishes to flush all files for the mounted
			// logical volume. The flush volume routine below should simply
			// walk through all of the open file streams, acquire the
			// FCB resource, and request the flush operation from the Cache
			// Manager. Basically, the sequence of operations listed below
			// for a single file should be executed on all open files.

			Ext2FlushLogicalVolume(PtrIrpContext, PtrIrp, PtrVCB);

			try_return();
		}

		if (!(PtrFCB->FCBFlags & EXT2_FCB_DIRECTORY)) 
		{
			// This is a regular file.
			ExAcquireResourceExclusiveLite(&(PtrReqdFCB->MainResource), TRUE);
			AcquiredFCB = TRUE;

			// Request the Cache Manager to perform a flush operation.
			// Further, instruct the Cache Manager that we wish to flush the
			// entire file stream.
			Ext2FlushAFile(PtrReqdFCB, &(PtrIrp->IoStatus));
			RC = PtrIrp->IoStatus.Status;
			// All done. You may want to also flush the directory entry for the
			// file stream at this time.

			// Some log-based FSD implementations may wish to flush their
			// log files at this time. Finally, you should update the time-stamp
			// values for the file stream appropriately. This would involve
			// obtaining the current time and modifying the appropriate directory
			// entry fields.
		}

		try_exit:

		if (AcquiredFCB) 
		{
			Ext2ReleaseResource(&(PtrReqdFCB->MainResource));
			DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released [Flush]", 0);
			DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [Flush]", 
				PtrReqdFCB->MainResource.ActiveCount, 
				PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, 
				PtrReqdFCB->MainResource.NumberOfSharedWaiters );

			AcquiredFCB = FALSE;
		}

		if (!PostRequest) 
		{
			PIO_STACK_LOCATION		PtrNextIoStackLocation = NULL;
			NTSTATUS						RC1 = STATUS_SUCCESS;

			// Send the request down at this point.
			// To do this, you must set the next IRP stack location, and
			// maybe set a completion routine.
			// Be careful about marking the IRP pending if the lower level
			// driver returned pending and you do have a completion routine!
			PtrNextIoStackLocation = IoGetNextIrpStackLocation(PtrIrp);
			*PtrNextIoStackLocation = *PtrIoStackLocation;

			// Set the completion routine to "eat-up" any
			// STATUS_INVALID_DEVICE_REQUEST error code returned by the lower
			// level driver.
			IoSetCompletionRoutine(PtrIrp, Ext2FlushCompletion, NULL, TRUE, TRUE, TRUE);

			/*
			 * The exception handlers propably masked out the
			 * fact that PtrVCB was never set.
			 * -- Filip Navara, 18/08/2004
			 */
			PtrVCB = PtrFCB->PtrVCB;
			RC1 = IoCallDriver(PtrVCB->TargetDeviceObject, PtrIrp);

			RC = ((RC1 == STATUS_INVALID_DEVICE_REQUEST) ? RC : RC1);
		}

	} finally {
		if (PostRequest) {
			// Nothing to lock now.
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
		} else {
			// Release the IRP context at this time.
  			Ext2ReleaseIrpContext(PtrIrpContext);
		}
	}

	return(RC);
}

/*************************************************************************
*
* Function: Ext2FlushAFile()
*
* Description:
*	Tell the Cache Manager to perform a flush.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2FlushAFile(
PtrExt2NTRequiredFCB	PtrReqdFCB,
PIO_STATUS_BLOCK		PtrIoStatus)
{
	CcFlushCache(&(PtrReqdFCB->SectionObject), NULL, 0, PtrIoStatus);
	return;
}

/*************************************************************************
*
* Function: Ext2FlushLogicalVolume()
*
* Description:
*	Flush everything beginning at root directory.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2FlushLogicalVolume(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp,
PtrExt2VCB					PtrVCB)
{
	BOOLEAN			AcquiredVCB = FALSE;

	try {
		ExAcquireResourceExclusiveLite(&(PtrVCB->VCBResource), TRUE);

		AcquiredVCB = TRUE;
		DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Acquired Ex [Flush] ", 0);

		// Go through the list of FCB's. You would probably
		// flush all of the files. Then, you could flush the
		// directories that you may have have pinned into memory.

		// NOTE: This function may also be invoked internally as part of
		// processing a shutdown request.

	} 
	finally 
	{
		if (AcquiredVCB) 
		{
			Ext2ReleaseResource(&(PtrVCB->VCBResource));
			DebugTrace(DEBUG_TRACE_MISC,  "*** VCB Released [Flush]", 0);
			DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Flush]", 
				PtrVCB->VCBResource.ActiveCount, 
				PtrVCB->VCBResource.NumberOfExclusiveWaiters, 
				PtrVCB->VCBResource.NumberOfSharedWaiters );
		}
	}

	return;
}


/*************************************************************************
*
* Function: Ext2FlushCompletion()
*
* Description:
*	Eat up any bad errors.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
NTSTATUS NTAPI Ext2FlushCompletion(
PDEVICE_OBJECT	PtrDeviceObject,
PIRP				PtrIrp,
PVOID				Context)
{
	if (PtrIrp->PendingReturned) {
		IoMarkIrpPending(PtrIrp);
	}

	if (PtrIrp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST) {
		// cannot do much here, can we?
		PtrIrp->IoStatus.Status = STATUS_SUCCESS;
	}

	return(STATUS_SUCCESS);
}

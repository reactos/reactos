/*************************************************************************
*
* File: cleanup.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Should contain code to handle the "Cleanup" dispatch entry point.
*	This file serves as a placeholder. Please update this file as part
*	of designing and implementing your FSD.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_CLEANUP
#define			DEBUG_LEVEL						(DEBUG_TRACE_CLEANUP)


/*************************************************************************
*
* Function: Ext2Cleanup()
*
* Description:
*	The I/O Manager will invoke this routine to handle a cleanup
*	request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*	to be deferred to a worker thread context)
*
* Return Value: Does not matter!
*
*************************************************************************/
NTSTATUS NTAPI Ext2Cleanup(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
    PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace( DEBUG_TRACE_IRP_ENTRY, "Cleanup IRP Received...", 0);

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

		RC = Ext2CommonCleanup(PtrIrpContext, Irp, TRUE);

	} 
	except( Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation() ) ) 
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
* Function: Ext2CommonCleanup()
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
* Return Value: Does not matter!
*
*************************************************************************/
NTSTATUS NTAPI Ext2CommonCleanup(
PtrExt2IrpContext			PtrIrpContext,
PIRP						PtrIrp,
BOOLEAN						FirstAttempt )
{

	NTSTATUS					RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION	PtrIoStackLocation = NULL;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;
	PtrExt2VCB				PtrVCB = NULL;
	PtrExt2NTRequiredFCB	PtrReqdFCB = NULL;
	PERESOURCE				PtrResourceAcquired = NULL;

	BOOLEAN					CompleteIrp = TRUE;
	BOOLEAN					PostRequest = FALSE;
	BOOLEAN					AcquiredVCB = FALSE;
	BOOLEAN					BlockForResource;
	int						i = 1;

	try 
	{
		// First, get a pointer to the current I/O stack location
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

		PtrFileObject = PtrIoStackLocation->FileObject;
		ASSERT(PtrFileObject);

		if( !PtrFileObject->FsContext2 )
		{
			//	This must be a Cleanup request received 
			//	as a result of IoCreateStreamFileObject
			//	Only such a File object would have a NULL CCB

			DebugTrace( DEBUG_TRACE_MISC, " === Cleanup with NULL CCB", 0);
			if( PtrFileObject )
			{
				DebugTrace( DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Cleanup]", PtrFileObject);
			}
			try_return();
		}
		

		Ext2GetFCB_CCB_VCB_FromFileObject ( 
			PtrFileObject, &PtrFCB, &PtrCCB, &PtrVCB );

		if( PtrFCB && PtrFCB->FCBName && PtrFCB->FCBName->ObjectName.Length && PtrFCB->FCBName->ObjectName.Buffer )
		//if( PtrFileObject->FileName.Length && PtrFileObject->FileName.Buffer )
		{
			DebugTrace( DEBUG_TRACE_FILE_NAME, " === Cleanup File Name : -%S-", PtrFCB->FCBName->ObjectName.Buffer );
		}
		else
		{
			DebugTrace( DEBUG_TRACE_FILE_NAME, " === Cleanup Volume", 0);
		}


		PtrVCB = (PtrExt2VCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
		ASSERT(PtrVCB);
		ASSERT(PtrVCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);
		
		//	(a) Acquiring the VCBResource Exclusively...
		//	This is done to synchronise with the close and cleanup routines...
		BlockForResource = !FirstAttempt;
		if( !FirstAttempt )
		{
			
			DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** Going into a block to acquire VCB Exclusively [Cleanup]", 0);
		}
		else
		{
			DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** Attempting to acquire VCB Exclusively [Cleanup]", 0);
		}

		if( PtrFileObject )
		{
			DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Cleanup]", PtrFileObject);
		}

		i = 1;
		while( !AcquiredVCB )
		{
			DebugTraceState("VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Cleanup]", PtrVCB->VCBResource.ActiveCount, PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
			if( ! ExAcquireResourceExclusiveLite( &(PtrVCB->VCBResource), FALSE ) )
			{
				DebugTrace( DEBUG_TRACE_RESOURCE_ACQUIRE, "*** VCB Acquisition FAILED [Cleanup]", 0);
				if( BlockForResource && i != 1000 )
				{
					LARGE_INTEGER Delay;
					Delay.QuadPart = -500 * i;
					KeDelayExecutionThread( KernelMode, FALSE, &Delay );
					DebugTrace(DEBUG_TRACE_RESOURCE_RETRY, "*** Retrying... after 50 * %ld ms [Cleanup]", i);
				}
				else
				{
					if( i == 1000 )
						DebugTrace(DEBUG_TRACE_RESOURCE_RETRY, "*** Reposting... [Cleanup]", 0 );
					PostRequest = TRUE;
					try_return( RC = STATUS_PENDING );
				}
			}
			else
			{
				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** VCB Acquired in [Cleanup]", 0);
				AcquiredVCB = TRUE;
			}
			i *= 10;
		}


		//	(b) Acquire the file (FCB) exclusively
		if( PtrFCB && PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB )
		{
			//	This FCB is an FCB indeed. ;)
			//	So acquiring it exclusively...
			//	This is done to synchronise with read/write routines...
			if( !FirstAttempt )
			{
				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** Going into a block to acquire FCB Exclusively [Cleanup]", 0);
			}
			else
			{
				DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** Attempting to acquire FCB Exclusively [Cleanup]", 0);
			}
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Cleanup]", PtrFileObject);
			}

			i = 1;
			while( !PtrResourceAcquired )
			{
				PtrReqdFCB = &(PtrFCB->NTRequiredFCB);
				DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [Cleanup]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
				if(! ExAcquireResourceExclusiveLite( &(PtrFCB->NTRequiredFCB.MainResource ), FALSE ) )
				{
					DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** FCB Acquisition FAILED [Cleanup]", 0);
					if( BlockForResource && i != 1000 )
					{
						LARGE_INTEGER Delay;
						Delay.QuadPart = -500 * i;
						KeDelayExecutionThread( KernelMode, FALSE, &Delay );
						DebugTrace(DEBUG_TRACE_RESOURCE_RETRY, "*** Retrying... after 50 * %ld ms [Cleanup]", i);
					}
					else
					{
						if( i == 1000 )
							DebugTrace(DEBUG_TRACE_RESOURCE_RETRY, "*** Reposting... [Cleanup]", 0 );
						PostRequest = TRUE;
						try_return( RC = STATUS_PENDING );
					}
				}
				else
				{
					DebugTrace(DEBUG_TRACE_RESOURCE_ACQUIRE, "*** FCB acquired [Cleanup]", 0);
					PtrResourceAcquired = & ( PtrFCB->NTRequiredFCB.MainResource );
				}
				i *= 10;
			}
	
			// (c) Flush file data to disk
			if ( PtrFileObject->PrivateCacheMap != NULL) 
			{
				IO_STATUS_BLOCK Status;
				CcFlushCache( PtrFileObject->SectionObjectPointer, NULL, 0, &Status );
			}

			// (d) Talk to the FSRTL package (if you use it) about pending oplocks.
			// (e) Notify the FSRTL package (if you use it) for use with pending
			//		 notification IRPs
			// (f) Unlock byte-range locks (if any were acquired by process)
			
			// (g) Attempting to update time stamp values
			//	Errors are ignored...
			//	Not considered as critical errors...

			/*
			if( PtrFCB->OpenHandleCount == 1 )
			{
				ULONG			CreationTime, AccessTime, ModificationTime;
				EXT2_INODE		Inode;

				CreationTime = (ULONG) ( (PtrFCB->CreationTime.QuadPart 
								- Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );
				AccessTime = (ULONG) ( (PtrFCB->LastAccessTime.QuadPart 
								- Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );
				ModificationTime = (ULONG) ( (PtrFCB->LastWriteTime.QuadPart
								- Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );
				if( NT_SUCCESS( Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
				{
					//	Update time stamps in the inode...
					Inode.i_ctime = CreationTime;
					Inode.i_atime = AccessTime;
					Inode.i_mtime = ModificationTime;

					//	Updating the inode...
					Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode );
				}
			}
			*/
			
			// (h) Inform the Cache Manager to uninitialize Cache Maps ...
			CcUninitializeCacheMap( PtrFileObject, NULL, NULL );

			// (i) Decrementing the Open Handle count...
			if( PtrFCB->OpenHandleCount )
			{
				InterlockedDecrement( &PtrFCB->OpenHandleCount );
			}
			else
			{
				Ext2BreakPoint();
			}

			PtrFCB->FCBFlags |= FO_CLEANUP_COMPLETE;
			
			DebugTrace(DEBUG_TRACE_REFERENCE, "^^^^^ReferenceCount  = 0x%lX [Cleanup]", PtrFCB->ReferenceCount );	
			DebugTrace(DEBUG_TRACE_REFERENCE, "^^^^^OpenHandleCount = 0x%lX [Cleanup]", PtrFCB->OpenHandleCount );

			//	(j) Remove share access...
			//		Will do that later ;)

			//	(k) Is this a close on delete file?
			//		If so, delete the file...
			if( Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_DELETE_ON_CLOSE) && 
				!PtrFCB->OpenHandleCount )
			{
				//
				//	Have to delete this file...
				//
				Ext2DeleteFile( PtrFCB, PtrIrpContext );
				PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart = 0;
				PtrFCB->INodeNo = 0;
			}
		}
		else
		{
			//	This must be a volume close...
			//	Just go ahead and complete this IRP...
			PtrVCB->VCBOpenCount--;
			DebugTrace(DEBUG_TRACE_MISC,   "VCB Cleanup Requested !!!", 0);
			CompleteIrp = TRUE;
		}
		
		try_return();

		try_exit:	NOTHING;

	}
	finally 
	{
		if(PtrResourceAcquired) 
		{
			Ext2ReleaseResource(PtrResourceAcquired);
			DebugTrace(DEBUG_TRACE_RESOURCE_RELEASE, "*** Resource Released [Cleanup]", 0);
			DebugTraceState( "Resource     AC:0x%LX   SW:0x%LX   EX:0x%LX   [Cleanup]", 
				PtrResourceAcquired->ActiveCount, 
				PtrResourceAcquired->NumberOfExclusiveWaiters, 
				PtrResourceAcquired->NumberOfSharedWaiters );

			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Cleanup]", PtrFileObject);
			}
			
		}

		if( AcquiredVCB )
		{
			ASSERT(PtrVCB);
			Ext2ReleaseResource(&(PtrVCB->VCBResource));
			DebugTrace(DEBUG_TRACE_RESOURCE_RELEASE, "*** VCB Released [Cleanup]", 0);
			DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Cleanup]", PtrVCB->VCBResource.ActiveCount, PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
			AcquiredVCB = FALSE;
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Cleanup]", PtrFileObject);
			}
			
		}

		if( PostRequest )
		{
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
		}
		if( RC != STATUS_PENDING )
		{
			Ext2ReleaseIrpContext( PtrIrpContext );
			// complete the IRP
			IoCompleteRequest( PtrIrp, IO_DISK_INCREMENT );
		}
	} // end of "finally" processing

	return(RC);
}

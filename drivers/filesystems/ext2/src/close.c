/*************************************************************************
*
* File: close.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Should contain code to handle the "Close" dispatch entry point.
*	This file serves as a placeholder. Please update this file as part
*	of designing and implementing your FSD.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_CLOSE

#define			DEBUG_LEVEL						(DEBUG_TRACE_CLOSE)


/*************************************************************************
*
* Function: Ext2Close()
*
* Description:
*	The I/O Manager will invoke this routine to handle a close
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
NTSTATUS NTAPI Ext2Close(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace(DEBUG_TRACE_IRP_ENTRY, "Close IRP Received...", 0);
	

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

		RC = Ext2CommonClose(PtrIrpContext, Irp, TRUE);

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
* Function: Ext2CommonClose()
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
NTSTATUS NTAPI Ext2CommonClose(
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

			DebugTrace( DEBUG_TRACE_SPECIAL, " === Close with NULL CCB", 0);
			if( PtrFileObject )
			{
				DebugTrace( DEBUG_TRACE_SPECIAL, "###### File Pointer 0x%LX [Close]", PtrFileObject);
			}
			try_return();
		}

		// Get the FCB and CCB pointers

		Ext2GetFCB_CCB_VCB_FromFileObject ( 
			PtrFileObject, &PtrFCB, &PtrCCB, &PtrVCB );

		PtrVCB = (PtrExt2VCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
		ASSERT( PtrVCB );

		if( PtrFCB && PtrFCB->FCBName && PtrFCB->FCBName->ObjectName.Length && PtrFCB->FCBName->ObjectName.Buffer )
		//if( PtrFileObject->FileName.Length && PtrFileObject->FileName.Buffer )
		{
			DebugTrace(DEBUG_TRACE_FILE_NAME, " === Close File Name : -%S-", PtrFCB->FCBName->ObjectName.Buffer );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_FILE_NAME,   " === Close File Name : -null-", 0);
		}

		//	(a) Acquiring the VCBResource Exclusively...
		//	This is done to synchronise with the close and cleanup routines...
//		if( ExTryToAcquireResourceExclusiveLite(&(PtrVCB->VCBResource) ) )

		BlockForResource = !FirstAttempt;
		if( !FirstAttempt )
		{
			DebugTrace(DEBUG_TRACE_MISC, "*** Going into a block to acquire VCB Exclusively [Close]", 0);
		}
		else
		{
			DebugTrace(DEBUG_TRACE_MISC, "*** Attempting to acquire VCB Exclusively [Close]", 0);
		}
		if( PtrFileObject )
		{
			DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Close]", PtrFileObject);
		}
		i = 1;
		while( !AcquiredVCB )
		{
			DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Close]", PtrVCB->VCBResource.ActiveCount, PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
			if(! ExAcquireResourceExclusiveLite( &(PtrVCB->VCBResource), FALSE ) )
			{
				DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Acquisition FAILED [Close]", 0);
				if( BlockForResource && i != 1000 )
				{
					LARGE_INTEGER Delay;
					
					//KeSetPriorityThread( PsGetCurrentThread(),LOW_REALTIME_PRIORITY	);

					Delay.QuadPart = -500 * i;
					KeDelayExecutionThread( KernelMode, FALSE, &Delay );
					DebugTrace(DEBUG_TRACE_MISC,  "*** Retrying... after 50 * %ld ms [Close]", i);
				}
				else
				{
					if( i == 1000 )
						DebugTrace(DEBUG_TRACE_MISC,  "*** Reposting... [Close]", 0 );
					PostRequest = TRUE;
					try_return( RC = STATUS_PENDING );
				}
			}
			else
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** VCB Acquired in [Close]", 0);
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
				DebugTrace(DEBUG_TRACE_MISC,   "*** Going into a block to acquire FCB Exclusively [Close]", 0);
			}
			else
			{
				DebugTrace(DEBUG_TRACE_MISC,  "*** Attempting to acquire FCB Exclusively [Close]", 0);
			}
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ,  "###### File Pointer 0x%LX [Close]", PtrFileObject);
			}
			i = 1;
			while( !PtrResourceAcquired )
			{
				PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

				DebugTraceState( "FCBMain   AC:0x%LX   SW:0x%LX   EX:0x%LX   [Close]", PtrReqdFCB->MainResource.ActiveCount, PtrReqdFCB->MainResource.NumberOfExclusiveWaiters, PtrReqdFCB->MainResource.NumberOfSharedWaiters );
				if(! ExAcquireResourceExclusiveLite( &(PtrFCB->NTRequiredFCB.MainResource ), FALSE ) )
				{
					DebugTrace(DEBUG_TRACE_MISC,   "*** FCB Acquisition FAILED [Close]", 0);
					if( BlockForResource && i != 1000 )
					{
						LARGE_INTEGER Delay;
						
						//KeSetPriorityThread( PsGetCurrentThread(),LOW_REALTIME_PRIORITY	);

						Delay.QuadPart = -500 * i;
						KeDelayExecutionThread( KernelMode, FALSE, &Delay );
						DebugTrace(DEBUG_TRACE_MISC,  "*** Retrying... after 50 * %ld ms [Close]", i);
					}
					else
					{
						if( i == 1000 )
							DebugTrace(DEBUG_TRACE_MISC,  "*** Reposting... [Close]", 0 );
						PostRequest = TRUE;
						try_return( RC = STATUS_PENDING );
					}
				}
				else
				{
					DebugTrace(DEBUG_TRACE_MISC,  "*** FCB acquired [Close]", 0);
					PtrResourceAcquired = & ( PtrFCB->NTRequiredFCB.MainResource );
				}
				i *= 10;
			}

			// (c) Delete the CCB structure (free memory)
			RemoveEntryList( &PtrCCB->NextCCB );
			Ext2ReleaseCCB( PtrCCB );
			PtrFileObject->FsContext2 = NULL;

			// (d) Decrementing the Reference Count...
			if( PtrFCB->ReferenceCount )
			{
				InterlockedDecrement( &PtrFCB->ReferenceCount );
			}
			else
			{
				Ext2BreakPoint();
			}	
			DebugTrace(DEBUG_TRACE_REFERENCE,  "^^^^^ReferenceCount = 0x%lX [Close]", PtrFCB->ReferenceCount );
			DebugTrace(DEBUG_TRACE_REFERENCE,  "^^^^^OpenHandleCount = 0x%lX [Close]", PtrFCB->OpenHandleCount );
			if( PtrFCB->ReferenceCount == 0 )
			{

				//	Attempting to update time stamp values
				//	Errors are ignored...
				//	Not considered as critical errors...
				
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
						Ext2WriteInode( NULL, PtrVCB, PtrFCB->INodeNo, &Inode );
					}
				}


				if( PtrFCB->INodeNo == EXT2_ROOT_INO )
				{
					//
					//	Root Directory FCB
					//	Preserve this
					//	FSD has a File Object for this FCB...
					//
					DebugTrace(DEBUG_TRACE_MISC,  "^^^^^Root Directory FCB ; leaveing it alone[Close]", 0);
					//	Do nothing...
					
				}
				else if( PtrFCB->DcbFcb.Dcb.PtrDirFileObject )
				{
					//
					//	If this is a FCB created on the FSD's initiative
					//	Leave it alone
					//
					DebugTrace(DEBUG_TRACE_MISC,  "^^^^^FCB Created  on the FSD's initiative; leaveing it alone[Close]", 0);
					if( !PtrFCB->ClosableFCBs.OnClosableFCBList )
					{
						InsertTailList( &PtrVCB->ClosableFCBs.ClosableFCBListHead,
							&PtrFCB->ClosableFCBs.ClosableFCBList );
						PtrVCB->ClosableFCBs.Count++;

						PtrFCB->ClosableFCBs.OnClosableFCBList = TRUE;
					}
					
					if( PtrVCB->ClosableFCBs.Count > EXT2_MAXCLOSABLE_FCBS_UL )
					{
						PtrExt2FCB		PtrTempFCB = NULL;
						//	Checking if Closable FCBs are too many in number...
						//	Shouldn't block the 
						//	Should do this asynchronously...
						//	Maybe later...
						PLIST_ENTRY		PtrEntry = NULL;

						PtrEntry = RemoveHeadList( &PtrVCB->ClosableFCBs.ClosableFCBListHead );
						
						PtrTempFCB = CONTAINING_RECORD( PtrEntry, Ext2FCB, ClosableFCBs.ClosableFCBList );
						if( Ext2CloseClosableFCB( PtrTempFCB ) )
						{
							DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [Close]", PtrTempFCB );
							ExFreePool( PtrTempFCB );
							PtrVCB->ClosableFCBs.Count--;
						}
						else
						{
							//	Put the FCB back in the list...
							InsertHeadList( &PtrVCB->ClosableFCBs.ClosableFCBListHead,
								&PtrTempFCB->ClosableFCBs.ClosableFCBList );
						}
						DebugTrace( DEBUG_TRACE_SPECIAL, "ClosableFCBs Count = %ld [Close]", PtrVCB->ClosableFCBs.Count );
					}
				}
				else
				{
					//	Remove this FCB as well...
					DebugTrace(DEBUG_TRACE_MISC,  "^^^^^Deleting FCB  [Close]", 0);
					RemoveEntryList( &PtrFCB->NextFCB );

					if ( PtrResourceAcquired ) 
					{
						Ext2ReleaseResource(PtrResourceAcquired);
						DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released [Close]", 0);
						DebugTraceState( "Resource     AC:0x%LX   SW:0x%LX   EX:0x%LX   [Close]", 
							PtrResourceAcquired->ActiveCount, 
							PtrResourceAcquired->NumberOfExclusiveWaiters, 
							PtrResourceAcquired->NumberOfSharedWaiters );

						if( PtrFileObject )
						{
							DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Close]", PtrFileObject);
						}
						PtrResourceAcquired = FALSE;
					}
					Ext2ReleaseFCB( PtrFCB );
				}

			}
			CompleteIrp = TRUE;
		}
		else
		{
			//	This must be a volume close...
			//	What do I do now? ;)
			DebugTrace(DEBUG_TRACE_MISC,   "VCB Close Requested !!!", 0);
			CompleteIrp = TRUE;
		}
		try_return();
		
		try_exit:	NOTHING;

	} 
	finally 
	{
		if ( PtrResourceAcquired ) 
		{
			Ext2ReleaseResource(PtrResourceAcquired);
			DebugTrace(DEBUG_TRACE_MISC,  "*** FCB Released [Close]", 0);
			DebugTraceState( "Resource     AC:0x%LX   SW:0x%LX   EX:0x%LX   [Close]", 
				PtrResourceAcquired->ActiveCount, 
				PtrResourceAcquired->NumberOfExclusiveWaiters, 
				PtrResourceAcquired->NumberOfSharedWaiters );

			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Close]", PtrFileObject);
			}
			PtrResourceAcquired = FALSE;
		}

		if (AcquiredVCB) 
		{
			ASSERT(PtrVCB);
			Ext2ReleaseResource(&(PtrVCB->VCBResource));
			DebugTraceState( "VCB       AC:0x%LX   SW:0x%LX   EX:0x%LX   [Close]", PtrVCB->VCBResource.ActiveCount, PtrVCB->VCBResource.NumberOfExclusiveWaiters, PtrVCB->VCBResource.NumberOfSharedWaiters );
			DebugTrace(DEBUG_TRACE_MISC,   "*** VCB Released [Close]", 0);

			AcquiredVCB = FALSE;
			if( PtrFileObject )
			{
				DebugTrace(DEBUG_TRACE_FILE_OBJ, "###### File Pointer 0x%LX [Close]", PtrFileObject);
			}
			
		}

		if( PostRequest )
		{
			RC = Ext2PostRequest(PtrIrpContext, PtrIrp);
		}
		else if( CompleteIrp && RC != STATUS_PENDING )
		{
			// complete the IRP
			IoCompleteRequest( PtrIrp, IO_DISK_INCREMENT );

			Ext2ReleaseIrpContext( PtrIrpContext );
		}

	} // end of "finally" processing

	return(RC);
}

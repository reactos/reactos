/*************************************************************************
*
* File: misc.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	This file contains some miscellaneous support routines.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_MISC

#define			DEBUG_LEVEL						( DEBUG_TRACE_MISC )

/*************************************************************************
*
* Function: Ext2InitializeZones()
*
* Description:
*	Allocates some memory for global zones used to allocate FSD structures.
*	Either all memory will be allocated or we will back out gracefully.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2InitializeZones(
void)
{
	NTSTATUS			RC = STATUS_SUCCESS;
	uint32				SizeOfZone = Ext2GlobalData.DefaultZoneSizeInNumStructs;
	uint32				SizeOfObjectNameZone = 0;
	uint32				SizeOfCCBZone = 0;
	uint32				SizeOfFCBZone = 0;
	uint32				SizeOfByteLockZone = 0;
	uint32				SizeOfIrpContextZone = 0;

	try {

		// initialize the spinlock protecting the zones
		KeInitializeSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock));

		// determine memory requirements
		switch (MmQuerySystemSize()) {
		case MmSmallSystem:
			// this is just for illustration purposes. I will multiply
			//	number of structures with some arbitrary amount depending
			//	upon available memory in the system ... You should choose a
			// more intelligent method suitable to your memory consumption
			// and the amount of memory available.
			SizeOfObjectNameZone = (2 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2ObjectName))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfCCBZone = (2 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2CCB))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfFCBZone = (2 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2FCB))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfByteLockZone = (2 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2FileLockInfo))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfIrpContextZone = (2 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2IrpContext))) + sizeof(ZONE_SEGMENT_HEADER);
			break;
		case MmMediumSystem:
			SizeOfObjectNameZone = (4 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2ObjectName))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfCCBZone = (4 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2CCB))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfFCBZone = (4 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2FCB))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfByteLockZone = (4 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2FileLockInfo))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfIrpContextZone = (4 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2IrpContext))) + sizeof(ZONE_SEGMENT_HEADER);
			break;
		case MmLargeSystem:
			SizeOfObjectNameZone = (8 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2ObjectName))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfCCBZone = (8 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2CCB))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfFCBZone = (8 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2FCB))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfByteLockZone = (8 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2FileLockInfo))) + sizeof(ZONE_SEGMENT_HEADER);
			SizeOfIrpContextZone = (8 * SizeOfZone * Ext2QuadAlign(sizeof(Ext2IrpContext))) + sizeof(ZONE_SEGMENT_HEADER);
			break;
		}

		// typical NT methodology (at least until *someone* exposed the "difference" between a server and workstation ;-)
		if (MmIsThisAnNtAsSystem()) {
			SizeOfObjectNameZone *= EXT2_NTAS_MULTIPLE;
			SizeOfCCBZone *= EXT2_NTAS_MULTIPLE;
			SizeOfFCBZone *= EXT2_NTAS_MULTIPLE;
			SizeOfByteLockZone *= EXT2_NTAS_MULTIPLE;
			SizeOfIrpContextZone *= EXT2_NTAS_MULTIPLE;
		}

		// allocate memory for each of the zones and initialize the	zones ...
		if (!(Ext2GlobalData.ObjectNameZone = Ext2AllocatePool(NonPagedPool, SizeOfObjectNameZone ))) {
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}

		if (!(Ext2GlobalData.CCBZone = Ext2AllocatePool(NonPagedPool, SizeOfCCBZone ))) {
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}

		if (!(Ext2GlobalData.FCBZone = Ext2AllocatePool(NonPagedPool, SizeOfFCBZone ))) {
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}

		if (!(Ext2GlobalData.ByteLockZone = Ext2AllocatePool(NonPagedPool, SizeOfByteLockZone ))) {
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}

		if (!(Ext2GlobalData.IrpContextZone = Ext2AllocatePool(NonPagedPool, SizeOfIrpContextZone ))) {
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}

		// initialize each of the zone headers ...
		if (!NT_SUCCESS(RC = ExInitializeZone(&(Ext2GlobalData.ObjectNameZoneHeader),
					Ext2QuadAlign(sizeof(Ext2ObjectName)),
					Ext2GlobalData.ObjectNameZone, SizeOfObjectNameZone))) {
			// failed the initialization, leave ...
			try_return(RC);
		}

		if (!NT_SUCCESS(RC = ExInitializeZone(&(Ext2GlobalData.CCBZoneHeader),
					Ext2QuadAlign(sizeof(Ext2CCB)),
					Ext2GlobalData.CCBZone,
					SizeOfCCBZone))) {
			// failed the initialization, leave ...
			try_return(RC);
		}

		if (!NT_SUCCESS(RC = ExInitializeZone(&(Ext2GlobalData.FCBZoneHeader),
					Ext2QuadAlign(sizeof(Ext2FCB)),
					Ext2GlobalData.FCBZone,
					SizeOfFCBZone))) {
			// failed the initialization, leave ...
			try_return(RC);
		}

		if (!NT_SUCCESS(RC = ExInitializeZone(&(Ext2GlobalData.ByteLockZoneHeader),
					Ext2QuadAlign(sizeof(Ext2FileLockInfo)),
					Ext2GlobalData.ByteLockZone,
					SizeOfByteLockZone))) {
			// failed the initialization, leave ...
			try_return(RC);
		}

		if (!NT_SUCCESS(RC = ExInitializeZone(&(Ext2GlobalData.IrpContextZoneHeader),
					Ext2QuadAlign(sizeof(Ext2IrpContext)),
					Ext2GlobalData.IrpContextZone,
					SizeOfIrpContextZone))) {
			// failed the initialization, leave ...
			try_return(RC);
		}

		try_exit:	NOTHING;

	} finally {
		if (!NT_SUCCESS(RC)) {
			// invoke the destroy routine now ...
			Ext2DestroyZones();
		} else {
			// mark the fact that we have allocated zones ...
			Ext2SetFlag(Ext2GlobalData.Ext2Flags, EXT2_DATA_FLAGS_ZONES_INITIALIZED);
		}
	}

	return(RC);
}


/*************************************************************************
*
* Function: Ext2DestroyZones()
*
* Description:
*	Free up the previously allocated memory. NEVER do this once the
*	driver has been successfully loaded.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2DestroyZones(
void)
{
	try {
		// free up each of the pools
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", Ext2GlobalData.ObjectNameZone);
		ExFreePool(Ext2GlobalData.ObjectNameZone);
		
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", Ext2GlobalData.CCBZone);
		ExFreePool(Ext2GlobalData.CCBZone);

		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", Ext2GlobalData.FCBZone);
		ExFreePool(Ext2GlobalData.FCBZone);

		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", Ext2GlobalData.ByteLockZone);
		ExFreePool(Ext2GlobalData.ByteLockZone);

		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", Ext2GlobalData.IrpContextZone);
		ExFreePool(Ext2GlobalData.IrpContextZone);
	} 
	finally 
	{
		Ext2ClearFlag(Ext2GlobalData.Ext2Flags, EXT2_DATA_FLAGS_ZONES_INITIALIZED);
	}

	return;
}


/*************************************************************************
*
* Function: Ext2IsIrpTopLevel()
*
* Description:
*	Helps the FSD determine who the "top level" caller is for this
*	request. A request can originate directly from a user process
*	(in which case, the "top level" will be NULL when this routine
*	is invoked), OR the user may have originated either from the NT
*	Cache Manager/VMM ("top level" may be set), or this could be a
*	recursion into our code in which we would have set the "top level"
*	field the last time around.
*
* Expected Interrupt Level (for execution) :
*
*  whatever level a particular dispatch routine is invoked at.
*
* Return Value: TRUE/FALSE (TRUE if top level was NULL when routine invoked)
*
*************************************************************************/
BOOLEAN NTAPI Ext2IsIrpTopLevel(
PIRP			Irp)			// the IRP sent to our dispatch routine
{
	BOOLEAN			ReturnCode = FALSE;

	if (IoGetTopLevelIrp() == NULL) 
	{
 		// OK, so we can set ourselves to become the "top level" component
		IoSetTopLevelIrp( Irp );
		ReturnCode = TRUE;
	}

	return(ReturnCode);
}


/*************************************************************************
*
* Function: Ext2ExceptionFilter()
*
* Description:
*	This routines allows the driver to determine whether the exception
*	is an "allowed" exception i.e. one we should not-so-quietly consume
*	ourselves, or one which should be propagated onwards in which case
*	we will most likely bring down the machine.
*
*	This routine employs the services of FsRtlIsNtstatusExpected(). This
*	routine returns a BOOLEAN result. A RC of FALSE will cause us to return
*	EXCEPTION_CONTINUE_SEARCH which will probably cause a panic.
*	The FsRtl.. routine returns FALSE iff exception values are (currently) :
*		STATUS_DATATYPE_MISALIGNMENT	||	STATUS_ACCESS_VIOLATION	||
*		STATUS_ILLEGAL_INSTRUCTION	||	STATUS_INSTRUCTION_MISALIGNMENT
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: EXCEPTION_EXECUTE_HANDLER/EXECEPTION_CONTINUE_SEARCH
*
*************************************************************************/
long NTAPI Ext2ExceptionFilter(
PtrExt2IrpContext			PtrIrpContext,
PEXCEPTION_POINTERS			PtrExceptionPointers )
{
	long							ReturnCode = EXCEPTION_EXECUTE_HANDLER;
	NTSTATUS						ExceptionCode = STATUS_SUCCESS;

	// figure out the exception code
	ExceptionCode = PtrExceptionPointers->ExceptionRecord->ExceptionCode;

	if ((ExceptionCode == STATUS_IN_PAGE_ERROR) && (PtrExceptionPointers->ExceptionRecord->NumberParameters >= 3)) 
	{
		ExceptionCode = PtrExceptionPointers->ExceptionRecord->ExceptionInformation[2];
	}

	if (PtrIrpContext) 
	{
		PtrIrpContext->SavedExceptionCode = ExceptionCode;
		Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_EXCEPTION);
	}

	// check if we should propagate this exception or not
	if (!(FsRtlIsNtstatusExpected(ExceptionCode))) 
	{
		// we are not ok, propagate this exception.
		//	NOTE: we will bring down the machine ...
		ReturnCode = EXCEPTION_CONTINUE_SEARCH;

		// better free up the IrpContext now ...
		if (PtrIrpContext) 
		{
			Ext2ReleaseIrpContext(PtrIrpContext);
		}
	}

	// if you wish to perform some special processing when
	//	not propagating the exception, set up the state for
	//	special processing now ...

	// return the appropriate code
	return(ReturnCode);
}

/*************************************************************************
*
* Function: Ext2ExceptionHandler()
*
* Description:
*	One of the routines in the FSD or in the modules we invoked encountered
*	an exception. We have decided that we will "handle" the exception.
*	Therefore we will prevent the machine from a panic ...
*	You can do pretty much anything you choose to in your commercial
*	driver at this point to ensure a graceful exit. In the sample
*	driver, I will simply free up the IrpContext (if any), set the
*	error code in the IRP and complete the IRP at this time ...
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: Error code
*
*************************************************************************/
NTSTATUS NTAPI Ext2ExceptionHandler(
PtrExt2IrpContext				PtrIrpContext,
PIRP								Irp)
{
	NTSTATUS						RC;

	ASSERT(Irp);

	if (PtrIrpContext) 
	{
		RC = PtrIrpContext->SavedExceptionCode;
		// Free irp context here
		Ext2ReleaseIrpContext(PtrIrpContext);
	} 
	else 
	{
		// must be insufficient resources ...?
		RC = STATUS_INSUFFICIENT_RESOURCES;
	}

	// set the error code in the IRP
	Irp->IoStatus.Status = RC;
	Irp->IoStatus.Information = 0;

	// complete the IRP
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(RC);
}

/*************************************************************************
*
* Function: Ext2LogEvent()
*
* Description:
*	Log a message in the NT Event Log. This is a rather simplistic log
*	methodology since you can potentially utilize the event log to
*	provide a lot of information to the user (and you should too!)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2LogEvent(
NTSTATUS					Ext2EventLogId,		// the Ext2 private message id
NTSTATUS					RC)						// any NT error code we wish to log ...
{
	try
	{

		// Implement a call to IoAllocateErrorLogEntry() followed by a call
		// to IoWriteErrorLogEntry(). You should note that the call to IoWriteErrorLogEntry()
		// will free memory for the entry once the write completes (which in actuality
		// is an asynchronous operation).

	} 
	except (EXCEPTION_EXECUTE_HANDLER) 
	{
		// nothing really we can do here, just do not wish to crash ...
		NOTHING;
	}

	return;
}

/*************************************************************************
*
* Function: Ext2AllocateObjectName()
*
* Description:
*	Allocate a new ObjectName structure to represent an open on-disk object.
*	Also initialize the ObjectName structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the ObjectName structure OR NULL.
*
*************************************************************************/
PtrExt2ObjectName NTAPI Ext2AllocateObjectName(
void)
{
	PtrExt2ObjectName			PtrObjectName = NULL;
	BOOLEAN						AllocatedFromZone = TRUE;
	//KIRQL							CurrentIrql;
/*
	// first, try to allocate out of the zone
	KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
	if (!ExIsFullZone(&(Ext2GlobalData.ObjectNameZoneHeader))) {
		// we have enough memory
		PtrObjectName = (PtrExt2ObjectName)ExAllocateFromZone(&(Ext2GlobalData.ObjectNameZoneHeader));

		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} else {
		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);

		// if we failed to obtain from the zone, get it directly from the VMM
*/
		PtrObjectName = (PtrExt2ObjectName)Ext2AllocatePool(NonPagedPool, Ext2QuadAlign(sizeof(Ext2ObjectName)) );
		AllocatedFromZone = FALSE;
/*
	}
*/
	// if we could not obtain the required memory, bug-check.
	//	Do NOT do this in your commercial driver, instead handle the error gracefully ...
	if (!PtrObjectName) 
	{
		Ext2Panic(STATUS_INSUFFICIENT_RESOURCES, Ext2QuadAlign(sizeof(Ext2ObjectName)), 0);
	}

	// zero out the allocated memory block
	RtlZeroMemory( PtrObjectName, Ext2QuadAlign(sizeof(Ext2ObjectName)) );

	// set up some fields ...
	PtrObjectName->NodeIdentifier.NodeType	= EXT2_NODE_TYPE_OBJECT_NAME;
	PtrObjectName->NodeIdentifier.NodeSize	= Ext2QuadAlign(sizeof(Ext2ObjectName));


	if (!AllocatedFromZone) 
	{
		Ext2SetFlag(PtrObjectName->ObjectNameFlags, EXT2_OB_NAME_NOT_FROM_ZONE);
	}

	return(PtrObjectName);
}


/*************************************************************************
*
* Function: Ext2ReleaseObjectName()
*
* Description:
*	Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2ReleaseObjectName(
PtrExt2ObjectName				PtrObjectName)
{
#ifdef USE_ZONES
	KIRQL							CurrentIrql;
#endif

	ASSERT(PtrObjectName);
	PtrObjectName->NodeIdentifier.NodeType = EXT2_NODE_TYPE_FREED;
#ifdef USE_ZONES

	// give back memory either to the zone or to the VMM
	if (!(PtrObjectName->ObjectNameFlags & EXT2_OB_NAME_NOT_FROM_ZONE)) 
	{
		// back to the zone
		KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
		ExFreeToZone(&(Ext2GlobalData.ObjectNameZoneHeader), PtrObjectName);
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} 
	else 
	{
#endif
	
	Ext2DeallocateUnicodeString( & PtrObjectName->ObjectName );

	DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrObjectName);
	ExFreePool(PtrObjectName);

#ifdef USE_ZONES	
	}
#endif
	return;
}

/*************************************************************************
*
* Function: Ext2AllocateCCB()
*
* Description:
*	Allocate a new CCB structure to represent an open on-disk object.
*	Also initialize the CCB structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the CCB structure OR NULL.
*
*************************************************************************/
PtrExt2CCB NTAPI Ext2AllocateCCB(
void)
{
	PtrExt2CCB					PtrCCB = NULL;
	BOOLEAN						AllocatedFromZone = TRUE;
#ifdef USE_ZONES
	KIRQL							CurrentIrql;
#endif


#ifdef USE_ZONES
   // first, try to allocate out of the zone
	KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
	if (!ExIsFullZone(&(Ext2GlobalData.CCBZoneHeader))) 
	{
		// we have enough memory
		PtrCCB = (PtrExt2CCB)ExAllocateFromZone(&(Ext2GlobalData.CCBZoneHeader));

		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} 
	else 
	{
		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
		// if we failed to obtain from the zone, get it directly from the VMM
#endif

		PtrCCB = (PtrExt2CCB)Ext2AllocatePool(NonPagedPool, Ext2QuadAlign(sizeof(Ext2CCB)) );
		AllocatedFromZone = FALSE;

#ifdef USE_ZONES
	}
#endif

	// if we could not obtain the required memory, bug-check.
	//	Do NOT do this in your commercial driver, instead handle the error gracefully ...
	if (!PtrCCB) 
	{
		Ext2Panic(STATUS_INSUFFICIENT_RESOURCES, Ext2QuadAlign(sizeof(Ext2CCB)), 0);
	}

	// zero out the allocated memory block
	RtlZeroMemory(PtrCCB, Ext2QuadAlign(sizeof(Ext2CCB)));

	// set up some fields ...
	PtrCCB->NodeIdentifier.NodeType	= EXT2_NODE_TYPE_CCB;
	PtrCCB->NodeIdentifier.NodeSize	= Ext2QuadAlign(sizeof(Ext2CCB));


	if (!AllocatedFromZone) 
	{
		Ext2SetFlag(PtrCCB->CCBFlags, EXT2_CCB_NOT_FROM_ZONE);
	}

	return(PtrCCB);
}

/*************************************************************************
*
* Function: Ext2ReleaseCCB()
*
* Description:
*	Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2ReleaseCCB(
PtrExt2CCB						PtrCCB)
{
#ifdef USE_ZONES
	KIRQL							CurrentIrql;
#endif

	ASSERT( PtrCCB );
	if(PtrCCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_CCB)
	{
		Ext2Panic( PtrCCB, PtrCCB->NodeIdentifier.NodeType, EXT2_NODE_TYPE_CCB )	;
	}

	Ext2DeallocateUnicodeString( &PtrCCB->DirectorySearchPattern );
	Ext2DeallocateUnicodeString( &PtrCCB->AbsolutePathName );
	Ext2DeallocateUnicodeString( &PtrCCB->RenameLinkTargetFileName );
	

#ifdef USE_ZONES
	
	// give back memory either to the zone or to the VMM
	if (!(PtrCCB->CCBFlags & EXT2_CCB_NOT_FROM_ZONE)) 
	{
		// back to the zone
		KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
		ExFreeToZone(&(Ext2GlobalData.CCBZoneHeader), PtrCCB);
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	}
	else
	{
#endif
		PtrCCB->NodeIdentifier.NodeType = EXT2_NODE_TYPE_FREED;
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrCCB);
		ExFreePool(PtrCCB);

#ifdef USE_ZONES
	}
#endif

	return;
}

/*************************************************************************
*
* Function: Ext2AllocateFCB()
*
* Description:
*	Allocate a new FCB structure to represent an open on-disk object.
*	Also initialize the FCB structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the FCB structure OR NULL.
*
*************************************************************************/
PtrExt2FCB NTAPI Ext2AllocateFCB(
void)
{
	PtrExt2FCB					PtrFCB = NULL;
	BOOLEAN						AllocatedFromZone = TRUE;
#ifdef USE_ZONES
	KIRQL						CurrentIrql;
#endif

	// first, try to allocate out of the zone
#ifdef USE_ZONES

	KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
	if (!ExIsFullZone(&(Ext2GlobalData.FCBZoneHeader))) {
		// we have enough memory
		PtrFCB = (PtrExt2FCB)ExAllocateFromZone(&(Ext2GlobalData.FCBZoneHeader));

		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} else {
		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
#endif
		// if we failed to obtain from the zone, get it directly from the VMM
		PtrFCB = (PtrExt2FCB)Ext2AllocatePool(NonPagedPool, Ext2QuadAlign(sizeof(Ext2FCB)) );
		AllocatedFromZone = FALSE;

#ifdef USE_ZONES
	}
#endif

	// if we could not obtain the required memory, bug-check.
	//	Do NOT do this in your commercial driver, instead handle the error gracefully ...
	if (!PtrFCB) 
	{
		Ext2Panic(STATUS_INSUFFICIENT_RESOURCES, Ext2QuadAlign(sizeof(Ext2FCB)), 0);
	}

	// zero out the allocated memory block
	RtlZeroMemory(PtrFCB, Ext2QuadAlign(sizeof(Ext2FCB)));

	// set up some fields ...
	PtrFCB->NodeIdentifier.NodeType	= EXT2_NODE_TYPE_FCB;
	PtrFCB->NodeIdentifier.NodeSize	= Ext2QuadAlign(sizeof(Ext2FCB));


	if (!AllocatedFromZone) 
	{
		Ext2SetFlag(PtrFCB->FCBFlags, EXT2_FCB_NOT_FROM_ZONE);
	}

	return(PtrFCB);
}


/*************************************************************************
*
* Function: Ext2CreateNewFCB()
*
* Description:
*	We want to create a new FCB. We will also create a new CCB (presumably)
*	later. Simply allocate a new FCB structure and initialize fields
*	appropriately.
*	This function also takes the file size values that the caller must
*	have obtained and	will set the file size fields appropriately in the
*	CommonFCBHeader.
*	Finally, this routine will initialize the FileObject structure passed
*	in to this function. If you decide to fail the call later, remember
*	to uninitialize the fields.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the FCB structure OR NULL.
*
*************************************************************************/
NTSTATUS NTAPI Ext2CreateNewFCB(
PtrExt2FCB				*ReturnedFCB,
LARGE_INTEGER			AllocationSize,
LARGE_INTEGER			EndOfFile,
PFILE_OBJECT			PtrFileObject,
PtrExt2VCB				PtrVCB,
PtrExt2ObjectName		PtrObjectName)
{
	NTSTATUS							RC = STATUS_SUCCESS;
	
	PtrExt2FCB						PtrFCB = NULL;
	PtrExt2NTRequiredFCB			PtrReqdFCB = NULL;
	PFSRTL_COMMON_FCB_HEADER	PtrCommonFCBHeader = NULL;

	ASSERT( PtrVCB );

	try 
	{
		if( !PtrFileObject )
		{
			PtrFCB = Ext2GetUsedFCB( PtrVCB );

		}
		else
		{
			// Obtain a new FCB structure.
			// The function Ext2AllocateFCB() will obtain a new structure either
			// from a zone or from memory requested directly from the VMM.
			PtrFCB = Ext2AllocateFCB();
		}
		if (!PtrFCB) 
		{
			// Assume lack of memory.
			try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
		}

		// Initialize fields required to interface with the NT Cache Manager.
		// Note that the returned structure has already been zeroed. This means
		// that the SectionObject structure has been zeroed which is a
		// requirement for newly created FCB structures.
		PtrReqdFCB = &(PtrFCB->NTRequiredFCB);

		// Initialize the MainResource and PagingIoResource structures now.
		ExInitializeResourceLite(&(PtrReqdFCB->MainResource));
		Ext2SetFlag(PtrFCB->FCBFlags, EXT2_INITIALIZED_MAIN_RESOURCE);

		ExInitializeResourceLite(&(PtrReqdFCB->PagingIoResource));
		Ext2SetFlag(PtrFCB->FCBFlags, EXT2_INITIALIZED_PAGING_IO_RESOURCE);

		// Start initializing the fields contained in the CommonFCBHeader.
		PtrCommonFCBHeader = &(PtrReqdFCB->CommonFCBHeader);

		// Disallow fast-IO for now.
		PtrCommonFCBHeader->IsFastIoPossible = FastIoIsNotPossible;

		// Initialize the MainResource and PagingIoResource pointers in
		// the CommonFCBHeader structure to point to the ERESOURCE structures we
		// have allocated and already initialized above.
		PtrCommonFCBHeader->Resource = &(PtrReqdFCB->MainResource);
		PtrCommonFCBHeader->PagingIoResource = &(PtrReqdFCB->PagingIoResource);

		// Ignore the Flags field in the CommonFCBHeader for now. Part 3
		// of the book describes it in greater detail.

		// Initialize the file size values here.
		PtrCommonFCBHeader->AllocationSize = AllocationSize;
		PtrCommonFCBHeader->FileSize = EndOfFile;

		// The following will disable ValidDataLength support. However, your
		// FSD may choose to support this concept.
		PtrCommonFCBHeader->ValidDataLength.LowPart = 0xFFFFFFFF;
		PtrCommonFCBHeader->ValidDataLength.HighPart = 0x7FFFFFFF;

		//	Initialize other fields for the FCB here ...
		PtrFCB->PtrVCB = PtrVCB;

		// caller MUST ensure that VCB has been acquired exclusively
		InsertTailList(&(PtrVCB->FCBListHead), &(PtrFCB->NextFCB));
	
		
		InitializeListHead(&(PtrFCB->CCBListHead));

		// Initialize fields contained in the file object now.
		if( PtrFileObject )
		{
			PtrFileObject->PrivateCacheMap = NULL;
			// Note that we could have just as well taken the value of PtrReqdFCB
			// directly below. The bottom line however is that the FsContext
			// field must point to a FSRTL_COMMON_FCB_HEADER structure.
			PtrFileObject->FsContext = (void *)(PtrCommonFCBHeader);
			PtrFileObject->SectionObjectPointer = &(PtrFCB->NTRequiredFCB.SectionObject) ;
		}

		//	Initialising the object name...
		PtrFCB->FCBName = PtrObjectName;

		//	Returning the FCB...
		*ReturnedFCB = PtrFCB;
		try_exit:	NOTHING;
	}

	finally 
	{
		
	}

	return(RC);
}


/*************************************************************************
*
* Function: Ext2ReleaseFCB()
*
* Description:
*	Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2ReleaseFCB(
PtrExt2FCB						PtrFCB)
{
	//KIRQL							CurrentIrql;

	AssertFCB( PtrFCB );

	if( PtrFCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB )
	{
		Ext2Panic( PtrFCB, PtrFCB->NodeIdentifier.NodeType, EXT2_NODE_TYPE_FCB )	;
	}


	PtrFCB->NodeIdentifier.NodeType = EXT2_NODE_TYPE_FREED;
	
	/*
	  // give back memory either to the zone or to the VMM
	if (!(PtrFCB->FCBFlags & EXT2_FCB_NOT_FROM_ZONE)) 
	{
		// back to the zone
		KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
		ExFreeToZone(&(Ext2GlobalData.FCBZoneHeader), PtrFCB);
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} 
	else 
	{
	*/

	ExDeleteResourceLite( &PtrFCB->NTRequiredFCB.MainResource );
	ExDeleteResourceLite( &PtrFCB->NTRequiredFCB.PagingIoResource );

	RemoveEntryList(&(PtrFCB->NextFCB));

	if( PtrFCB->FCBName )
	{
		Ext2ReleaseObjectName( PtrFCB->FCBName );
	}

	DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrFCB);
	ExFreePool(PtrFCB);
	
	/*
	}
	*/

	return;
}

/*************************************************************************
*
* Function: Ext2AllocateByteLocks()
*
* Description:
*	Allocate a new byte range lock structure and initialize it to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the Ext2ByteLocks structure OR NULL.
*
*************************************************************************/
PtrExt2FileLockInfo NTAPI Ext2AllocateByteLocks(
void)
{
	PtrExt2FileLockInfo		PtrByteLocks = NULL;
	BOOLEAN						AllocatedFromZone = TRUE;
   KIRQL							CurrentIrql;

	// first, try to allocate out of the zone
	KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
	if (!ExIsFullZone(&(Ext2GlobalData.ByteLockZoneHeader))) 
	{
		// we have enough memory
		PtrByteLocks = (PtrExt2FileLockInfo)ExAllocateFromZone(&(Ext2GlobalData.ByteLockZoneHeader));

		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} 
	else 
	{
		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);

		// if we failed to obtain from the zone, get it directly from the VMM
		PtrByteLocks = (PtrExt2FileLockInfo)Ext2AllocatePool(NonPagedPool, Ext2QuadAlign(sizeof(Ext2FileLockInfo)) );
		AllocatedFromZone = FALSE;
	}

	// if we could not obtain the required memory, bug-check.
	//	Do NOT do this in your commercial driver, instead handle the error gracefully ...
	if (!PtrByteLocks) 
	{
		Ext2Panic(STATUS_INSUFFICIENT_RESOURCES, Ext2QuadAlign(sizeof(Ext2FileLockInfo)), 0);
	}

	// zero out the allocated memory block
	RtlZeroMemory(PtrByteLocks, Ext2QuadAlign(sizeof(PtrExt2FileLockInfo)));

	if (!AllocatedFromZone) 
	{
		Ext2SetFlag(PtrByteLocks->FileLockFlags, EXT2_BYTE_LOCK_NOT_FROM_ZONE);
	}

	return(PtrByteLocks);
}

/*************************************************************************
*
* Function: Ext2ReleaseByteLocks()
*
* Description:
*	Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2ReleaseByteLocks(
PtrExt2FileLockInfo					PtrByteLocks)
{
	KIRQL							CurrentIrql;

	ASSERT(PtrByteLocks);

	// give back memory either to the zone or to the VMM
	if (!(PtrByteLocks->FileLockFlags & EXT2_BYTE_LOCK_NOT_FROM_ZONE)) {
		// back to the zone
		KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
      ExFreeToZone(&(Ext2GlobalData.ByteLockZoneHeader), PtrByteLocks);
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} 
	else 
	{
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrByteLocks);
		ExFreePool(PtrByteLocks);
	}

	return;
}


/*************************************************************************
*
* Function: Ext2AllocateIrpContext()
*
* Description:
*	The sample FSD creates an IRP context for each request received. This
*	routine simply allocates (and initializes to NULL) a Ext2IrpContext
*	structure.
*	Most of the fields in the context structure are then initialized here.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the IrpContext structure OR NULL.
*
*************************************************************************/
PtrExt2IrpContext NTAPI Ext2AllocateIrpContext(
PIRP					Irp,
PDEVICE_OBJECT		PtrTargetDeviceObject)
{
	PtrExt2IrpContext			PtrIrpContext = NULL;
	BOOLEAN						AllocatedFromZone = TRUE;
	//KIRQL							CurrentIrql;
	PIO_STACK_LOCATION		PtrIoStackLocation = NULL;

	/* 
	//	Allocation from zone not done at present...
	
	// first, try to allocate out of the zone
	KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
	if (!ExIsFullZone(&(Ext2GlobalData.IrpContextZoneHeader))) {
		// we have enough memory
		PtrIrpContext = (PtrExt2IrpContext)ExAllocateFromZone(&(Ext2GlobalData.IrpContextZoneHeader));

		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} else {
		// release the spinlock
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	
	  
	
	 
		// if we failed to obtain from the zone, get it directly from the VMM
		PtrIrpContext = (PtrExt2IrpContext)Ext2AllocatePool(NonPagedPool, Ext2QuadAlign(sizeof(Ext2IrpContext)) );
        AllocatedFromZone = FALSE;
	}
	
	//No Zone handling for now
	*/

	PtrIrpContext = (PtrExt2IrpContext)Ext2AllocatePool(NonPagedPool, Ext2QuadAlign(sizeof(Ext2IrpContext)) );
    AllocatedFromZone = FALSE;

	// if we could not obtain the required memory, bug-check.
	//	Do NOT do this in your commercial driver, instead handle	the error gracefully ...
	if (!PtrIrpContext) 
	{
		Ext2Panic(STATUS_INSUFFICIENT_RESOURCES, Ext2QuadAlign(sizeof(Ext2IrpContext)), 0);
	}

	// zero out the allocated memory block
	RtlZeroMemory(PtrIrpContext, Ext2QuadAlign(sizeof(Ext2IrpContext)));

	// set up some fields ...
	PtrIrpContext->NodeIdentifier.NodeType	= EXT2_NODE_TYPE_IRP_CONTEXT;
	PtrIrpContext->NodeIdentifier.NodeSize	= Ext2QuadAlign(sizeof(Ext2IrpContext));


	PtrIrpContext->Irp = Irp;
	PtrIrpContext->TargetDeviceObject = PtrTargetDeviceObject;

	// copy over some fields from the IRP and set appropriate flag values
	if (Irp) 
	{
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
		ASSERT(PtrIoStackLocation);

		PtrIrpContext->MajorFunction = PtrIoStackLocation->MajorFunction;
		PtrIrpContext->MinorFunction = PtrIoStackLocation->MinorFunction;

		// Often, a FSD cannot honor a request for asynchronous processing
		// of certain critical requests. For example, a "close" request on
		// a file object can typically never be deferred. Therefore, do not
		// be surprised if sometimes your FSD (just like all other FSD
		// implementations on the Windows NT system) has to override the flag
		// below.
		if( PtrIoStackLocation->FileObject )
		{
			if (IoIsOperationSynchronous(Irp) ) 
			{
				Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_CAN_BLOCK);
			}
		}
		else
		{
			Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_CAN_BLOCK);
		}
	}

	if (!AllocatedFromZone) 
	{
		Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_NOT_FROM_ZONE);
	}

	// Are we top-level ? This information is used by the dispatching code
	// later (and also by the FSD dispatch routine)
	if (IoGetTopLevelIrp() != Irp) 
	{
		// We are not top-level. Note this fact in the context structure
		Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_NOT_TOP_LEVEL);
	}

	InitializeListHead( &PtrIrpContext->SavedBCBsListHead );

	return(PtrIrpContext);
}


/*************************************************************************
*
* Function: Ext2ReleaseIrpContext()
*
* Description:
*	Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2ReleaseIrpContext(
PtrExt2IrpContext					PtrIrpContext)
{
	KIRQL							CurrentIrql;

	ASSERT(PtrIrpContext);

	//	Flush the saved BCBs...
	Ext2FlushSavedBCBs( PtrIrpContext );

	// give back memory either to the zone or to the VMM
	if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_NOT_FROM_ZONE)) 
	{
		// back to the zone
		KeAcquireSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), &CurrentIrql);
		ExFreeToZone(&(Ext2GlobalData.IrpContextZoneHeader), PtrIrpContext);
		KeReleaseSpinLock(&(Ext2GlobalData.ZoneAllocationSpinLock), CurrentIrql);
	} 
	else 
	{
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrIrpContext);
		ExFreePool(PtrIrpContext);
	}

	return;
}

/*************************************************************************
*
* Function: Ext2PostRequest()
*
* Description:
*	Queue up a request for deferred processing (in the context of a system
*	worker thread). The caller must have locked the user buffer (if required)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_PENDING
*
*************************************************************************/
NTSTATUS NTAPI Ext2PostRequest(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp)
{
	NTSTATUS			RC = STATUS_PENDING;

	DebugTrace(DEBUG_TRACE_ASYNC, " === Asynchronous request. Deferring processing", 0);
	
	// mark the IRP pending
	IoMarkIrpPending(PtrIrp);

	// queue up the request
	ExInterlockedInsertTailList(
		&Ext2GlobalData.ThreadQueue.ThreadQueueListHead, 
		&PtrIrpContext->ThreadQueueListEntry,
		&Ext2GlobalData.ThreadQueue.SpinLock );

	KeSetEvent( &Ext2GlobalData.ThreadQueue.QueueEvent, 0, FALSE );
				

/*****************		not using system worker threads		*****************
	ExInitializeWorkItem(&(PtrIrpContext->WorkQueueItem), Ext2CommonDispatch, PtrIrpContext);
	ExQueueWorkItem( &( PtrIrpContext->WorkQueueItem ), DelayedWorkQueue );
	//	CriticalWorkQueue 
*****************************************************************************/

	// return status pending
	return(RC);
}


/*************************************************************************
*
* Function: Ext2CommonDispatch()
*
* Description:
*	The common dispatch routine invoked in the context of a system worker
*	thread. All we do here is pretty much case off the major function
*	code and invoke the appropriate FSD dispatch routine for further
*	processing.
*
* Expected Interrupt Level (for execution) :
*
*	IRQL PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2CommonDispatch(
			void		*Context )	// actually an IRPContext structure
{
	NTSTATUS						RC = STATUS_SUCCESS;
	PtrExt2IrpContext			PtrIrpContext = NULL;
	PIRP							PtrIrp = NULL;

	// The context must be a pointer to an IrpContext structure
	PtrIrpContext = (PtrExt2IrpContext)Context;
	ASSERT(PtrIrpContext);

	// Assert that the Context is legitimate
	if ((PtrIrpContext->NodeIdentifier.NodeType != EXT2_NODE_TYPE_IRP_CONTEXT) || (PtrIrpContext->NodeIdentifier.NodeSize != Ext2QuadAlign(sizeof(Ext2IrpContext)))) 
	{
		// This does not look good
		Ext2Panic(EXT2_ERROR_INTERNAL_ERROR, PtrIrpContext->NodeIdentifier.NodeType, PtrIrpContext->NodeIdentifier.NodeSize);
	}

	//	Get a pointer to the IRP structure
	PtrIrp = PtrIrpContext->Irp;
	ASSERT(PtrIrp);

	// Now, check if the FSD was top level when the IRP was originally invoked
	// and set the thread context (for the worker thread) appropriately
	if (PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_NOT_TOP_LEVEL) 
	{
		// The FSD is not top level for the original request
		// Set a constant value in TLS to reflect this fact
		IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
	}

	// Since the FSD routine will now be invoked in the context of this worker
	// thread, we should inform the FSD that it is perfectly OK to block in
	// the context of this thread
	Ext2SetFlag(PtrIrpContext->IrpContextFlags, EXT2_IRP_CONTEXT_CAN_BLOCK);

	FsRtlEnterFileSystem();

	try
	{

		// Pre-processing has been completed; check the Major Function code value
		// either in the IrpContext (copied from the IRP), or directly from the
		//	IRP itself (we will need a pointer to the stack location to do that),
		//	Then, switch based on the value on the Major Function code
		switch (PtrIrpContext->MajorFunction) 
		{
		case IRP_MJ_CREATE:
			// Invoke the common create routine
			DebugTrace(DEBUG_TRACE_ASYNC,   " === Serviceing IRP_MJ_CREATE request asynchronously .", 0);
			(void)Ext2CommonCreate(PtrIrpContext, PtrIrp, FALSE);
			break;
		case IRP_MJ_READ:
			// Invoke the common read routine
			DebugTrace(DEBUG_TRACE_ASYNC,   " === Serviceing IRP_MJ_READ request asynchronously .", 0);
			(void)Ext2CommonRead(PtrIrpContext, PtrIrp, FALSE);
			break;
		case IRP_MJ_WRITE:
			// Invoke the common write routine
			DebugTrace(DEBUG_TRACE_ASYNC,   " === Serviceing IRP_MJ_WRITE request asynchronously .", 0);
			(void)Ext2CommonWrite(PtrIrpContext, PtrIrp );
			break;

		case IRP_MJ_CLEANUP:
			// Invoke the common read routine
			DebugTrace(DEBUG_TRACE_ASYNC,   " === Serviceing IRP_MJ_CLEANUP request asynchronously .", 0);
			(void)Ext2CommonCleanup(PtrIrpContext, PtrIrp, FALSE);
			break;
		case IRP_MJ_CLOSE:
			// Invoke the common read routine
			DebugTrace(DEBUG_TRACE_ASYNC,   " === Serviceing IRP_MJ_CLOSE request asynchronously .", 0);
			(void)Ext2CommonClose ( PtrIrpContext, PtrIrp, FALSE );
			break;

		// Continue with the remaining possible dispatch routines below ...
		default:
			// This is the case where we have an invalid major function
			DebugTrace(DEBUG_TRACE_ASYNC,   " === Serviceing asynchronous request. \nUnable to recoganise the IRP!!! How can this be!!!", 0);
			PtrIrp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			PtrIrp->IoStatus.Information = 0;
			
			Ext2BreakPoint();
	
			IoCompleteRequest(PtrIrp, IO_NO_INCREMENT);
			break;
		}
	} 
	except (Ext2ExceptionFilter(PtrIrpContext, GetExceptionInformation())) 
	{
		RC = Ext2ExceptionHandler(PtrIrpContext, PtrIrp);
		Ext2LogEvent(EXT2_ERROR_INTERNAL_ERROR, RC);
	}

	// Enable preemption
	FsRtlExitFileSystem();

	// Ensure that the "top-level" field is cleared
	IoSetTopLevelIrp(NULL);
	
	PsTerminateSystemThread( RC );


	return;
}

/*************************************************************************
*
* Function: Ext2InitializeVCB()
*
* Description:
*	Perform the initialization for a VCB structure.
*
* Expected Interrupt Level (for execution) :
*
*	IRQL PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2InitializeVCB(
PDEVICE_OBJECT			PtrVolumeDeviceObject,
PDEVICE_OBJECT			PtrTargetDeviceObject,
PVPB					PtrVPB,
PLARGE_INTEGER			AllocationSize )
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2VCB			PtrVCB = NULL;
	BOOLEAN				VCBResourceInitialized = FALSE;

	PtrVCB = (PtrExt2VCB)(PtrVolumeDeviceObject->DeviceExtension);

	// Zero it out (typically this has already been done by the I/O
	// Manager but it does not hurt to do it again)!
	RtlZeroMemory(PtrVCB, sizeof(Ext2VCB));

	// Initialize the signature fields
	PtrVCB->NodeIdentifier.NodeType = EXT2_NODE_TYPE_VCB;
	PtrVCB->NodeIdentifier.NodeSize = sizeof(Ext2VCB);

	// Initialize the ERESOURCE objects.
	RC = ExInitializeResourceLite(&(PtrVCB->VCBResource));
	RC = ExInitializeResourceLite(&(PtrVCB->PagingIoResource));

	ASSERT(NT_SUCCESS(RC));
	VCBResourceInitialized = TRUE;

	PtrVCB->TargetDeviceObject = PtrTargetDeviceObject;

	PtrVCB->VCBDeviceObject = PtrVolumeDeviceObject;

	PtrVCB->PtrVPB = PtrVPB;

	// Initialize the list anchor (head) for some lists in this VCB.
	InitializeListHead(&(PtrVCB->FCBListHead));
	InitializeListHead(&(PtrVCB->NextNotifyIRP));
	InitializeListHead(&(PtrVCB->VolumeOpenListHead));
	InitializeListHead(&(PtrVCB->ClosableFCBs.ClosableFCBListHead));
	PtrVCB->ClosableFCBs.Count = 0;

	// Initialize the notify IRP list mutex
	KeInitializeMutex(&(PtrVCB->NotifyIRPMutex), 0);

	// Set the initial file size values appropriately. Note that your FSD may
	// wish to guess at the initial amount of information you would like to
	// read from the disk until you have really determined that this a valid
	// logical volume (on disk) that you wish to mount.
	PtrVCB->CommonVCBHeader.AllocationSize.QuadPart = AllocationSize->QuadPart;

	PtrVCB->CommonVCBHeader.FileSize.QuadPart = AllocationSize->QuadPart;
	// You typically do not want to bother with valid data length callbacks
	// from the Cache Manager for the file stream opened for volume metadata
	// information
	PtrVCB->CommonVCBHeader.ValidDataLength.LowPart = 0xFFFFFFFF;
	PtrVCB->CommonVCBHeader.ValidDataLength.HighPart = 0x7FFFFFFF;

	PtrVCB->CommonVCBHeader.IsFastIoPossible = FastIoIsNotPossible;
	
	PtrVCB->CommonVCBHeader.Resource = &(PtrVCB->VCBResource);
	PtrVCB->CommonVCBHeader.PagingIoResource = &(PtrVCB->PagingIoResource);;

	// Create a stream file object for this volume.
	PtrVCB->PtrStreamFileObject = IoCreateStreamFileObject(NULL,
												PtrVCB->PtrVPB->RealDevice);
	ASSERT(PtrVCB->PtrStreamFileObject);

	// Initialize some important fields in the newly created file object.
	PtrVCB->PtrStreamFileObject->FsContext = (void *)(&PtrVCB->CommonVCBHeader);
	PtrVCB->PtrStreamFileObject->FsContext2 = NULL;
	PtrVCB->PtrStreamFileObject->SectionObjectPointer = &(PtrVCB->SectionObject);

	PtrVCB->PtrStreamFileObject->Vpb = PtrVPB;
	PtrVCB->PtrStreamFileObject->ReadAccess = TRUE;
	PtrVCB->PtrStreamFileObject->WriteAccess = TRUE;

	// Link this chap onto the global linked list of all VCB structures.
	DebugTrace(DEBUG_TRACE_MISC,   "*** Attempting to acquire Global Resource Exclusively [FileInfo]", 0);
	ExAcquireResourceExclusiveLite(&(Ext2GlobalData.GlobalDataResource), TRUE);
	InsertTailList(&(Ext2GlobalData.NextVCB), &(PtrVCB->NextVCB));
	DebugTrace(DEBUG_TRACE_MISC,   "*** Global Resource Acquired [FileInfo]", 0);


	
	// Initialize caching for the stream file object.
	CcInitializeCacheMap(PtrVCB->PtrStreamFileObject, (PCC_FILE_SIZES)(&(PtrVCB->CommonVCBHeader.AllocationSize)),
								TRUE,		// We will use pinned access.
								&(Ext2GlobalData.CacheMgrCallBacks), PtrVCB );

	
	Ext2ReleaseResource(&(Ext2GlobalData.GlobalDataResource));
	DebugTrace(DEBUG_TRACE_MISC,   "*** Global Resource Released[FileInfo]", 0);

	// Mark the fact that this VCB structure is initialized.
	Ext2SetFlag(PtrVCB->VCBFlags, EXT2_VCB_FLAGS_VCB_INITIALIZED);
	PtrVCB->PtrGroupDescriptors = NULL;
	PtrVCB->NoOfGroups = 0;
	return;
}



/*************************************************************************
*
* Function: Ext2CompleteRequest()
*
* Description:
*	This routine completes a Irp.
*
* Expected Interrupt Level (for execution) :
*
*   ???
* 
* Arguments:
*
*   Irp - Supplies the Irp being processed
*
*   Status - Supplies the status to complete the Irp with
*
* Return Value: none
*
*************************************************************************/
void NTAPI Ext2CompleteRequest(
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )
{
	//
    //  If we have an Irp then complete the irp.
    //

    if (Irp != NULL) 
	{

        //
        //  We got an error, so zero out the information field before
        //  completing the request if this was an input operation.
        //  Otherwise IopCompleteRequest will try to copy to the user's buffer.
        //

        if ( NT_ERROR(Status) &&
             FlagOn(Irp->Flags, IRP_INPUT_OPERATION) ) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }
    return;
}


/*************************************************************************
*
* Function: Ext2CreateNewCCB()
*
* Description:
*	We want to create a new CCB. 
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the CCB structure OR NULL.
*
*************************************************************************/
NTSTATUS NTAPI Ext2CreateNewCCB(
	PtrExt2CCB				*ReturnedCCB,
	PtrExt2FCB				PtrFCB,
	PFILE_OBJECT			PtrFileObject )
{
	PtrExt2CCB  PtrCCB;
	NTSTATUS RC = STATUS_SUCCESS;

	try 
	{

		PtrCCB = Ext2AllocateCCB();
		if (!PtrFCB) 
		{
			// Assume lack of memory.
			try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
		}
		PtrCCB->PtrFCB = PtrFCB;
		
		PtrCCB->PtrFileObject = PtrFileObject;
		PtrCCB->CurrentByteOffset.QuadPart = 0;

		if( PtrFCB->ClosableFCBs.OnClosableFCBList )
		{
			//	This FCB was on the Closable List...
			//	Taking it off the list...
			//
			RemoveEntryList( &PtrFCB->ClosableFCBs.ClosableFCBList );
			PtrFCB->ClosableFCBs.OnClosableFCBList = FALSE;
			PtrFCB->PtrVCB->ClosableFCBs.Count --;
		}

		InterlockedIncrement( &PtrFCB->ReferenceCount );
		InterlockedIncrement( &PtrFCB->OpenHandleCount );

		InsertTailList( &( PtrFCB->CCBListHead ), &(PtrCCB->NextCCB));

		*ReturnedCCB = PtrCCB;
		try_exit:	NOTHING;
	} 
	finally 
	{
		
	}

	return(RC);
}


/*************************************************************************
*
* Function: Ext2DenyAccess()
*
* Description:
*	We want to deny access to an IRP
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: NTSTATUS - STATUS_ACCESS_DENIED (always)
*
*************************************************************************/
NTSTATUS NTAPI Ext2DenyAccess( IN PIRP Irp )
{
    ASSERT( Irp );

	//	Just return Access Denied
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
	IoCompleteRequest( Irp, IO_DISK_INCREMENT );
	
	DebugTrace(DEBUG_TRACE_MISC,   "DENYING ACCESS (this will do for now!)...", 0);
	
	return STATUS_ACCESS_DENIED;
}



/*************************************************************************
*
* Function: Ext2GetFCB_CCB_VCB_FromFileObject()
*
* Description:
*	This routine retrieves the FCB, CCB and VCB from the File Object...
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: NTSTATUS - STATUS_SUCCESS(always)
*
*************************************************************************/
NTSTATUS NTAPI Ext2GetFCB_CCB_VCB_FromFileObject (
	IN PFILE_OBJECT			PtrFileObject,
	OUT PtrExt2FCB				*PPtrFCB,
	OUT PtrExt2CCB				*PPtrCCB,
	OUT PtrExt2VCB				*PPtrVCB	)
{
		(*PPtrCCB) = (PtrExt2CCB)(PtrFileObject->FsContext2);
		if( *PPtrCCB )
		{
			ASSERT((*PPtrCCB)->NodeIdentifier.NodeType == EXT2_NODE_TYPE_CCB);
			(*PPtrFCB) = (*PPtrCCB)->PtrFCB;

			ASSERT((*PPtrFCB));
			
			if ((*PPtrFCB)->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB)
			{
				(*PPtrVCB) = (PtrExt2VCB)(*PPtrFCB);
				AssertVCB( (*PPtrVCB) );

				//	No FCB
				(*PPtrFCB) = NULL;
				//found a VCB
			}
			else
			{
				AssertFCB( (*PPtrFCB) );
				(*PPtrVCB) = (*PPtrFCB)->PtrVCB;
				AssertVCB( (*PPtrVCB) );

			}
		}
		else
		{
			//	PtrFileObject->FsContext points to NTRequiredFCB
			(*PPtrFCB) = CONTAINING_RECORD( PtrFileObject->FsContext, Ext2FCB, NTRequiredFCB );
			ASSERT((*PPtrFCB));
			//(*PPtrFCB) = PtrFileObject->FsContext;

			if ((*PPtrFCB)->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB)
			{
				//	Making sure I got it right...
				AssertFCB( *PPtrFCB );
				(*PPtrVCB) = (*PPtrFCB)->PtrVCB;
				AssertVCB( *PPtrVCB );
			}
			else
			{
				//	This should be a VCB

				(*PPtrVCB) = CONTAINING_RECORD( PtrFileObject->FsContext, Ext2VCB, CommonVCBHeader );
				AssertVCB( *PPtrVCB );
				
				//	No FCB
				(*PPtrFCB) = NULL;
				//found a VCB
			}
			
		}
	return STATUS_SUCCESS;
}


void NTAPI Ext2CopyUnicodeString( PUNICODE_STRING  PtrDestinationString, PUNICODE_STRING PtrSourceString )
{
	int Count;
	//	Allcating space for Destination...
	PtrDestinationString->Length = PtrSourceString->Length;
	PtrDestinationString->MaximumLength = Ext2QuadAlign( PtrSourceString->Length + 2 );
	PtrDestinationString->Buffer = Ext2AllocatePool( NonPagedPool, PtrDestinationString->MaximumLength );

	//	RtlCopyUnicodeString( PtrDestinationString, PtrSourceString );

	for( Count = 0 ; Count < (PtrSourceString->Length/2) ; Count++ )
	{
		PtrDestinationString->Buffer[Count] = PtrSourceString->Buffer[Count];
	}
	PtrDestinationString->Buffer[Count] = 0;

}

void NTAPI Ext2CopyWideCharToUnicodeString( 
	PUNICODE_STRING  PtrDestinationString, 
	PCWSTR PtrSourceString )
{
	
	int Count; 

	//	Determining length...
	for( Count = 0 ; PtrSourceString[Count] != 0 ; Count++ );
	
	//	Allcating space for Destination...
	PtrDestinationString->Length = Count * 2;
	PtrDestinationString->MaximumLength = Ext2QuadAlign( Count * 2 + 2 );	
	PtrDestinationString->Buffer = Ext2AllocatePool( NonPagedPool, PtrDestinationString->MaximumLength  );
	
	//	Copying the string over...
	for( Count = 0 ; ; Count++ )
	{
		PtrDestinationString->Buffer[Count] = PtrSourceString[Count];
		if( PtrSourceString[Count] == 0 )
			break;
	}
}


void NTAPI Ext2CopyCharToUnicodeString( 
	PUNICODE_STRING  PtrDestinationString, 
	PCSTR PtrSourceString,
	USHORT SourceStringLength )
{
	int Count;
	//	Allcating space for Destination...
	PtrDestinationString->Length = SourceStringLength * 2;
	PtrDestinationString->MaximumLength = Ext2QuadAlign( SourceStringLength * 2 + 2 );	
	PtrDestinationString->Buffer = Ext2AllocatePool( NonPagedPool, PtrDestinationString->MaximumLength  );
	
	//	Copying the string over...
	for( Count = 0 ; Count < SourceStringLength ; Count++ )
	{
		PtrDestinationString->Buffer[Count] = PtrSourceString[Count];
	}
	PtrDestinationString->Buffer[Count] = 0;

}

void NTAPI Ext2CopyZCharToUnicodeString( PUNICODE_STRING  PtrDestinationString, PCSTR PtrSourceString )
{
	
	int Count; 

	//	Determining length...
	for( Count = 0 ; PtrSourceString[Count] != 0 ; Count++ );
	
	//	Allcating space for Destination...
	PtrDestinationString->Length = Count * 2;
	PtrDestinationString->MaximumLength = Ext2QuadAlign( Count * 2 + 2 );	
	PtrDestinationString->Buffer = Ext2AllocatePool( NonPagedPool, PtrDestinationString->MaximumLength  );
	
	//	Copying the string over...
	for( Count = 0 ; ; Count++ )
	{
		PtrDestinationString->Buffer[Count] = PtrSourceString[Count];
		if( PtrSourceString[Count] == 0 )
			break;
	}
}

void NTAPI Ext2ZerooutUnicodeString( PUNICODE_STRING PtrUnicodeString )
{
	PtrUnicodeString->Length = 0;
	PtrUnicodeString->MaximumLength =0;
	PtrUnicodeString->Buffer = 0;
}

void NTAPI Ext2DeallocateUnicodeString( PUNICODE_STRING PtrUnicodeString )
{
	if( PtrUnicodeString && PtrUnicodeString->Buffer )
	{
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrUnicodeString->Buffer );
		ExFreePool( PtrUnicodeString->Buffer );
	}
	PtrUnicodeString->Length = 0;
	PtrUnicodeString->MaximumLength =0;
	PtrUnicodeString->Buffer = 0;
}

PtrExt2FCB NTAPI Ext2GetUsedFCB( 
	PtrExt2VCB	PtrVCB )
{

	BOOLEAN			AllocatedFromZone = FALSE;
	PLIST_ENTRY		PtrEntry = NULL;
	PtrExt2FCB		PtrFCB = NULL;

	ASSERT( PtrVCB );
	if( PtrVCB->ClosableFCBs.Count < EXT2_MAXCLOSABLE_FCBS_LL )
	{
		//
		//	Too few Closable FCBs
		//	Will not reuse any FCBs 
		//	Allocating a new one
		//
		return Ext2AllocateFCB();
	}
	//
	//	Obtaining a used FCB...
	//
	
	//	Retrieving the first entry in the closable FCB list...

	PtrEntry = RemoveHeadList( &PtrVCB->ClosableFCBs.ClosableFCBListHead );
	
	PtrFCB = CONTAINING_RECORD( PtrEntry, Ext2FCB, ClosableFCBs.ClosableFCBList );

	//	Remembering if the FCB was allocated from the Zone...
	AllocatedFromZone = Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_NOT_FROM_ZONE );

	//	
	//	Close this FCB
	//
	if( !Ext2CloseClosableFCB( PtrFCB ) )
	{
		//	Couldn't close the FCB!!
		//	
		InsertHeadList( &PtrVCB->ClosableFCBs.ClosableFCBListHead,
			&PtrFCB->ClosableFCBs.ClosableFCBList );
		return Ext2AllocateFCB();
	}

	PtrVCB->ClosableFCBs.Count--;
	DebugTrace( DEBUG_TRACE_SPECIAL, "Count = %ld [Ext2GetUsedFCB]", PtrVCB->ClosableFCBs.Count );

	//
	//	Getting the FCB ready for reuse by 
	//	zeroing it out...
	//
	RtlZeroMemory(PtrFCB, Ext2QuadAlign(sizeof(Ext2FCB)));

	// set up some fields ...
	PtrFCB->NodeIdentifier.NodeType	= EXT2_NODE_TYPE_FCB;
	PtrFCB->NodeIdentifier.NodeSize	= Ext2QuadAlign(sizeof(Ext2FCB));


	if (!AllocatedFromZone) 
	{
		Ext2SetFlag(PtrFCB->FCBFlags, EXT2_FCB_NOT_FROM_ZONE);
	}
	
	return PtrFCB;
}

BOOLEAN NTAPI Ext2CloseClosableFCB( 
	PtrExt2FCB		PtrFCB)
{
	KIRQL			Irql = 0;
	PFILE_OBJECT	PtrFileObject = NULL;

	AssertFCB( PtrFCB );

	//	Attempting to acquire the FCB Exclusively...
	if(! ExAcquireResourceExclusiveLite( &(PtrFCB->NTRequiredFCB.MainResource ), FALSE ) )
	{
		Ext2BreakPoint();
		return  FALSE;
	}

	Irql = KeGetCurrentIrql( );

	if( PtrFCB->ReferenceCount )
	{
		//	How the hell can this happen!!!
		Ext2BreakPoint();
	}
	if( PtrFCB->OpenHandleCount )
	{
		//	How the hell can this happen!!!
		Ext2BreakPoint();
	}

	//	Deleting entry from VCB's FCB list...
	RemoveEntryList( &PtrFCB->NextFCB );

	PtrFCB->NodeIdentifier.NodeType = EXT2_NODE_TYPE_FREED;

	PtrFileObject = PtrFCB->DcbFcb.Dcb.PtrDirFileObject;

	if ( PtrFileObject )
	{
		//
		//	Clear the Cache Map...
		//
		if( PtrFileObject->PrivateCacheMap != NULL) 
		{
			IO_STATUS_BLOCK Status;
			DebugTrace( DEBUG_TRACE_SPECIAL, ">>.........Flushing cache.........<<", 0 );
			CcFlushCache( PtrFileObject->SectionObjectPointer, NULL, 0, &Status );
			CcUninitializeCacheMap( PtrFileObject, NULL, NULL );
		}
		//
		//	The File Object is no longer required...
		//	Close it by dereferenceing it!!!
		//
		PtrFileObject->FsContext	= NULL;
		PtrFileObject->FsContext2	= NULL;
		ObDereferenceObject( PtrFileObject );

		PtrFCB->DcbFcb.Dcb.PtrDirFileObject = NULL;
		PtrFileObject = NULL;
	}

	//	Uninitialize the Resources...
	ExDeleteResourceLite( &PtrFCB->NTRequiredFCB.MainResource );
	ExDeleteResourceLite( &PtrFCB->NTRequiredFCB.PagingIoResource );

	//
	//	Releasing the FCB Name Object...
	//
	if( PtrFCB->FCBName )
	{
		DebugTrace( DEBUG_TRACE_SPECIAL, "Reusing FCB - File Name %S", PtrFCB->FCBName->ObjectName.Buffer );
		Ext2ReleaseObjectName( PtrFCB->FCBName );
	}
	else
	{
		DebugTrace( DEBUG_TRACE_SPECIAL, "Reusing FCB - File Name *Unknown*", 0 );
	}
	return TRUE;
}


BOOLEAN NTAPI Ext2SaveBCB(
	PtrExt2IrpContext	PtrIrpContext,
	PBCB				PtrBCB,
	PFILE_OBJECT		PtrFileObject)
{
	PEXT2_SAVED_BCBS	PtrSavedBCB;
	PLIST_ENTRY			PtrEntry = NULL;

	if( !PtrIrpContext )
	{
		//
		//	NULL passed instead of the IRP Context
		//	This call should be ignored...
		//
		return TRUE;
	}

	if( !AssertBCB( PtrBCB ) )
	{
		DebugTrace( DEBUG_TRACE_MISC, "Not saving BCB!!! [Ext2SaveBCB]", 0 );
		return FALSE;
	}


	DebugTrace( DEBUG_TRACE_SPECIAL, "Saving BCB [Ext2SaveBCB]", 0 );

	//	Has the BCB been saved already?
	for( PtrEntry = PtrIrpContext->SavedBCBsListHead.Flink; 
			PtrEntry != &PtrIrpContext->SavedBCBsListHead; 
			PtrEntry = PtrEntry->Flink )
	{
		PtrSavedBCB = CONTAINING_RECORD( PtrEntry, EXT2_SAVED_BCBS, SavedBCBsListEntry );
		ASSERT( PtrSavedBCB );
		if( PtrSavedBCB->PtrBCB == PtrBCB )
		{

			//	A BCB for this file has already been saved for flushing...
			//	Won't resave this one...
			return TRUE;
		}
	}


	//	Reference the BCB 
	CcRepinBcb( PtrBCB );

	//	Now allocate a EXT2_SAVED_BCBS
	PtrSavedBCB = Ext2AllocatePool( NonPagedPool, 
					Ext2QuadAlign( sizeof( EXT2_SAVED_BCBS ) )  );
	if( !PtrSavedBCB )
		return FALSE;
	PtrSavedBCB->NodeIdentifier.NodeSize = sizeof( EXT2_SAVED_BCBS );
	PtrSavedBCB->NodeIdentifier.NodeType = EXT2_NODE_TYPE_SAVED_BCB;

	PtrSavedBCB->PtrBCB = PtrBCB;
	//	PtrSavedBCB->PtrFileObject = PtrFileObject;
	
	//	Now save it in the IRP Context
	InsertHeadList( &PtrIrpContext->SavedBCBsListHead, &PtrSavedBCB->SavedBCBsListEntry );
	
	PtrIrpContext->SavedCount++;
	//	Return success...
	return TRUE;

}


BOOLEAN NTAPI Ext2FlushSavedBCBs(
	PtrExt2IrpContext	PtrIrpContext )
{
	
	PLIST_ENTRY			PtrEntry = NULL;
	PEXT2_SAVED_BCBS	PtrSavedBCB = NULL;
	IO_STATUS_BLOCK		Status;
	BOOLEAN				RC = TRUE;

	if( !IsListEmpty( &PtrIrpContext->SavedBCBsListHead ) )
	{
		DebugTrace( DEBUG_TRACE_SPECIAL, "Flushing cache... - Ext2FlushSavedBCBs", 0 );
	}
	while( !IsListEmpty( &PtrIrpContext->SavedBCBsListHead ) )
 	{

		PtrEntry = RemoveTailList( &PtrIrpContext->SavedBCBsListHead );
		if( !PtrEntry )
		{
			//	No more entries left...
			break;
		}

		//	Get the Saved BCB
		PtrSavedBCB = CONTAINING_RECORD( PtrEntry, EXT2_SAVED_BCBS, SavedBCBsListEntry );
		if( PtrSavedBCB->NodeIdentifier.NodeType != EXT2_NODE_TYPE_SAVED_BCB )
		{
			//	Something is wrong...
			Ext2BreakPoint();
			return FALSE;
		}

		if( !AssertBCB( PtrSavedBCB->PtrBCB ) )
		{
			//	This BCB shouldn't have been saved in the first place...
			DebugTrace( DEBUG_TRACE_ERROR, "Unable to flush BCB - Skipping!!! [Ext2SaveBCB]", 0 );
			continue;
		}

		//	Unpin and Flush the cache...
		CcUnpinRepinnedBcb( PtrSavedBCB->PtrBCB, TRUE, &Status );
		
		if( !NT_SUCCESS( Status.Status ) )
		{
			//	Failure in flushing...
			DebugTrace( DEBUG_TRACE_SPECIAL, "Failure flushing cache - Ext2FlushSavedBCBs", 0 );
			RC = FALSE;
		}

		//	Release the Saved BCB
		PtrSavedBCB->NodeIdentifier.NodeType = EXT2_NODE_TYPE_INVALID;
		
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [misc]", PtrSavedBCB );
		ExFreePool( PtrSavedBCB );
		PtrSavedBCB = NULL;
		PtrIrpContext->SavedCount--;
	}

	return RC;
}

BOOLEAN NTAPI AssertBCB( PBCB	PtrBCB )
{
	PFILE_OBJECT		PtrFileObject = NULL;
	
	/*
	 * This routine is simplified version of the original
	 * AssertBCB and doesn't make any assumptions about
	 * the layout of undocumented BCB structure.
	 * -- Filip Navara, 18/08/2004
	 */

		PtrFileObject = CcGetFileObjectFromBcb ( PtrBCB );
		if( !PtrFileObject )
		{
			Ext2BreakPoint();
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}


ULONG NTAPI Ext2Align( ULONG NumberToBeAligned, ULONG Alignment )
{
	if( Alignment & ( Alignment - 1 ) )
	{
		//
		//	Alignment not a power of 2
		//	Just returning
		//
		return NumberToBeAligned;
	}
	if( ( NumberToBeAligned & ( Alignment - 1 ) ) != 0 )
	{
		NumberToBeAligned = NumberToBeAligned + Alignment;
		NumberToBeAligned = NumberToBeAligned & ( ~ (Alignment-1) );
	}
	return NumberToBeAligned;
}

LONGLONG NTAPI Ext2Align64( LONGLONG NumberToBeAligned, LONGLONG Alignment )
{
	if( Alignment & ( Alignment - 1 ) )
	{
		//
		//	Alignment not a power of 2
		//	Just returning
		//
		return NumberToBeAligned;
	}
	if( ( NumberToBeAligned & ( Alignment - 1 ) ) != 0 )
	{
		NumberToBeAligned = NumberToBeAligned + Alignment;
		NumberToBeAligned = NumberToBeAligned & ( ~ (Alignment-1) );
	}
	return NumberToBeAligned;
}


ULONG Ext2GetCurrentTime()
{
	LARGE_INTEGER  CurrentTime;
	ULONG Time;
	KeQuerySystemTime( &CurrentTime );
	Time = (ULONG) ( (CurrentTime.QuadPart - Ext2GlobalData.TimeDiff.QuadPart) / 10000000 );
	return Time;
}

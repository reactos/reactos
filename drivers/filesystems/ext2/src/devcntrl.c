/*************************************************************************
*
* File: devcntrl.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "Device IOCTL" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_DEVICE_CONTROL
#define			DEBUG_LEVEL						DEBUG_TRACE_DEVCTRL


#if(_WIN32_WINNT < 0x0400)
#define IOCTL_REDIR_QUERY_PATH   CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 99, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _QUERY_PATH_REQUEST 
{
    ULONG PathNameLength;
    PIO_SECURITY_CONTEXT SecurityContext;
    WCHAR FilePathName[1];
} QUERY_PATH_REQUEST, *PQUERY_PATH_REQUEST;

typedef struct _QUERY_PATH_RESPONSE 
{
    ULONG LengthAccepted;
} QUERY_PATH_RESPONSE, *PQUERY_PATH_RESPONSE;
#endif


/*************************************************************************
*
* Function: Ext2DeviceControl()
*
* Description:
*	The I/O Manager will invoke this routine to handle a Device IOCTL
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
NTSTATUS NTAPI Ext2DeviceControl(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
   PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	//	Ext2BreakPoint();
	
	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "Device Control IRP Received...", 0);

	FsRtlEnterFileSystem();
	ASSERT(DeviceObject);
	ASSERT(Irp);

	// set the top level context
	AreWeTopLevel = Ext2IsIrpTopLevel(Irp);

	try {

		// get an IRP context structure and issue the request
		PtrIrpContext = Ext2AllocateIrpContext(Irp, DeviceObject);
		ASSERT(PtrIrpContext);

		RC = Ext2CommonDeviceControl(PtrIrpContext, Irp);

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
* Function: Ext2CommonDeviceControl()
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
NTSTATUS NTAPI Ext2CommonDeviceControl(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp)
{
	NTSTATUS					RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION	PtrIoStackLocation = NULL;
	PIO_STACK_LOCATION	PtrNextIoStackLocation = NULL;
	PFILE_OBJECT			PtrFileObject = NULL;
	PtrExt2FCB				PtrFCB = NULL;
	PtrExt2CCB				PtrCCB = NULL;
	PtrExt2VCB				PtrVCB = NULL;
	ULONG						IoControlCode = 0;

	try 
	{
		// First, get a pointer to the current I/O stack location
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

		PtrFileObject = PtrIoStackLocation->FileObject;
		ASSERT(PtrFileObject);

		PtrCCB = (PtrExt2CCB)(PtrFileObject->FsContext2);
		ASSERT(PtrCCB);
		PtrFCB = PtrCCB->PtrFCB;
		ASSERT(PtrFCB);
		

		if( PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB )
		{
			PtrVCB = (PtrExt2VCB)(PtrFCB);
		}
		else
		{
			AssertFCB( PtrFCB );
			ASSERT(PtrFCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_FCB);
			PtrVCB = PtrFCB->PtrVCB;
		}

		// Get the IoControlCode value
		IoControlCode = PtrIoStackLocation->Parameters.DeviceIoControl.IoControlCode;

		// You may wish to allow only	volume open operations.

		// Invoke the lower level driver in the chain.
		PtrNextIoStackLocation = IoGetNextIrpStackLocation(PtrIrp);
		*PtrNextIoStackLocation = *PtrIoStackLocation;
		// Set a completion routine.
		IoSetCompletionRoutine(PtrIrp, Ext2DevIoctlCompletion, NULL, TRUE, TRUE, TRUE);
		// Send the request.
		RC = IoCallDriver(PtrVCB->TargetDeviceObject, PtrIrp);
	} 
	finally 
	{
		// Release the IRP context
		if (!(PtrIrpContext->IrpContextFlags & EXT2_IRP_CONTEXT_EXCEPTION)) 
		{
			// Free up the Irp Context
			Ext2ReleaseIrpContext(PtrIrpContext);
		}
	}

	return(RC);
}


/*************************************************************************
*
* Function: Ext2DevIoctlCompletion()
*
* Description:
*	Completion routine.
*	
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS
*
*************************************************************************/
NTSTATUS NTAPI Ext2DevIoctlCompletion(
PDEVICE_OBJECT			PtrDeviceObject,
PIRP						PtrIrp,
void						*Context)
{
	if (PtrIrp->PendingReturned) {
		IoMarkIrpPending(PtrIrp);
	}

	return(STATUS_SUCCESS);
}

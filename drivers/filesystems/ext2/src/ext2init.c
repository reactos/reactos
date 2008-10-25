/*************************************************************************
*
* File: ext2init.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*     This file contains the initialization code for the kernel mode
*     Ext2 FSD module. The DriverEntry() routine is called by the I/O
*     sub-system to initialize the FSD.
*
* Author: Manoj Paul Joseph
*
*************************************************************************/

 
#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_INIT
#define			DEBUG_LEVEL						(DEBUG_TRACE_INIT)

#define			EXT2_FS_NAME					L"\\ext2"

// global variables are declared here
Ext2Data					Ext2GlobalData;


/*************************************************************************
*
* Function: DriverEntry()
*
* Description:
*	This routine is the standard entry point for all kernel mode drivers.
*	The routine is invoked at IRQL PASSIVE_LEVEL in the context of a
*	system worker thread.
*	All FSD specific data structures etc. are initialized here.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (will cause driver to be unloaded).
*
*************************************************************************/
NTSTATUS NTAPI DriverEntry(
PDRIVER_OBJECT		DriverObject,		// created by the I/O sub-system
PUNICODE_STRING	RegistryPath)		// path to the registry key
{
	NTSTATUS		RC = STATUS_SUCCESS;
	UNICODE_STRING	DriverDeviceName;

#if 0
	Ext2BreakPoint();
#endif

	try 
	{
		try 
		{
			
			DebugTrace(DEBUG_TRACE_IRP_ENTRY, "Ext2 File System Driver Entry <<<<<<<", 0);
			// initialize the global data structure
			RtlZeroMemory(&Ext2GlobalData, sizeof(Ext2GlobalData));

			// initialize some required fields
			Ext2GlobalData.NodeIdentifier.NodeType = EXT2_NODE_TYPE_GLOBAL_DATA;
			Ext2GlobalData.NodeIdentifier.NodeSize = sizeof(Ext2GlobalData);

			// initialize the global data resource and remember the fact that
			//	the resource has been initialized
			RC = ExInitializeResourceLite(&(Ext2GlobalData.GlobalDataResource));
			ASSERT(NT_SUCCESS(RC));
			Ext2SetFlag(Ext2GlobalData.Ext2Flags, EXT2_DATA_FLAGS_RESOURCE_INITIALIZED);

			// keep a ptr to the driver object sent to us by the I/O Mgr
			Ext2GlobalData.Ext2DriverObject = DriverObject;

			// initialize the mounted logical volume list head
			InitializeListHead( &( Ext2GlobalData.NextVCB ) );

			// before we proceed with any more initialization, read in
			//	user supplied configurable values ...
			// if (!NT_SUCCESS(RC = Ext2ObtainRegistryValues(RegistryPath))) {
					// in your commercial driver implementation, it would be
					//	advisable for your driver to print an appropriate error
					//	message to the system error log before leaving
			//		try_return(RC);
			//	}

			// we should have the registry data (if any), allocate zone memory ...
			//	This is an example of when FSD implementations try to pre-allocate
			//	some fixed amount of memory to avoid internal fragmentation and/or waiting
			//	later during run-time ...

#ifdef USE_ZONES
			
			if (!NT_SUCCESS(RC = Ext2InitializeZones())) 
			{
				// we failed, print a message and leave ...
				try_return(RC);
			}
#endif

			//
			//	Initialize the Thread queue structure...
			//
			KeInitializeEvent( 
				&Ext2GlobalData.ThreadQueue.QueueEvent,
				SynchronizationEvent,
				FALSE
				);
			KeInitializeSpinLock( &Ext2GlobalData.ThreadQueue.SpinLock );
			InitializeListHead( &Ext2GlobalData.ThreadQueue.ThreadQueueListHead );

			//
			//	Done Initializing...
			//	Now Creating a worker thread to handle Worker threads...
			//
			PsCreateSystemThread( 
				&Ext2GlobalData.ThreadQueue.QueueHandlerThread, (ACCESS_MASK) 0L, 
				NULL, NULL, NULL, Ext2QueueHandlerThread, NULL );

			// initialize the IRP major function table, and the fast I/O table
			Ext2FsdInitializeFunctionPointers(DriverObject);

			// create a device object representing the driver itself
			//	so that requests can be targeted to the driver ...
			//	e.g. for a disk-based FSD, "mount" requests will be sent to
			//		  this device object by the I/O Manager.
			//		  For a redirector/server, you may have applications
			//		  send "special" IOCTL's using this device object ...
			RtlInitUnicodeString(&DriverDeviceName, EXT2_FS_NAME);
			if (!NT_SUCCESS(RC = IoCreateDevice(
					DriverObject,		// our driver object
					0,						// don't need an extension for this object
					&DriverDeviceName,// name - can be used to "open" the driver
					                  // see the book for alternate choices
					FILE_DEVICE_DISK_FILE_SYSTEM,
					0,						// no special characteristics
					  						// do not want this as an exclusive device, though you might
					FALSE,
					&(Ext2GlobalData.Ext2DeviceObject)))) 
			{
				// failed to create a device object, leave ...
				try_return(RC);
			}



			// register the driver with the I/O Manager, pretend as if this is
			//	a physical disk based FSD (or in order words, this FSD manages
			//	logical volumes residing on physical disk drives)
			IoRegisterFileSystem(Ext2GlobalData.Ext2DeviceObject);

			{
				TIME_FIELDS		TimeFields;

				TimeFields.Day = 1;
				TimeFields.Hour = 0;
				TimeFields.Milliseconds = 0;
				TimeFields.Minute = 0;
				TimeFields.Month = 1;
				TimeFields.Second = 0;
				TimeFields.Weekday = 0;
				TimeFields.Year = 1970;
				RtlTimeFieldsToTime( &TimeFields, &Ext2GlobalData.TimeDiff );

				/*
				Ext2GlobalData.TimeDiff.QuadPart = 0;
				RtlTimeToTimeFields( &Ext2GlobalData.TimeDiff,&TimeFields );
				TimeFields.Year = 2002;
				RtlTimeFieldsToTime( &TimeFields, &Ext2GlobalData.TimeDiff );
				*/

			}
		}
		except (EXCEPTION_EXECUTE_HANDLER) 
		{
			// we encountered an exception somewhere, eat it up
			RC = GetExceptionCode();
		}

		try_exit:	NOTHING;
	}
	finally 
	{
		// start unwinding if we were unsuccessful
		if (!NT_SUCCESS(RC)) 
		{

			// Now, delete any device objects, etc. we may have created
			if (Ext2GlobalData.Ext2DeviceObject) 
			{
				IoDeleteDevice(Ext2GlobalData.Ext2DeviceObject);
				Ext2GlobalData.Ext2DeviceObject = NULL;
			}

			// free up any memory we might have reserved for zones/lookaside
			//	lists
			if (Ext2GlobalData.Ext2Flags & EXT2_DATA_FLAGS_ZONES_INITIALIZED) 
			{
				Ext2DestroyZones();
			}

			// delete the resource we may have initialized
			if (Ext2GlobalData.Ext2Flags & EXT2_DATA_FLAGS_RESOURCE_INITIALIZED) 
			{
				// un-initialize this resource
				ExDeleteResourceLite(&(Ext2GlobalData.GlobalDataResource));
				Ext2ClearFlag(Ext2GlobalData.Ext2Flags, EXT2_DATA_FLAGS_RESOURCE_INITIALIZED);
			}
		}
	}

	return(RC);
}

/*************************************************************************
*
* Function: Ext2FsdInitializeFunctionPointers()
*
* Description:
*	Initialize the IRP... function pointer array in the driver object
*	structure. Also initialize the fast-io function ptr array ...
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2FsdInitializeFunctionPointers(
PDRIVER_OBJECT		DriverObject)		// created by the I/O sub-system
{
   PFAST_IO_DISPATCH	PtrFastIoDispatch = NULL;
	
	// initialize the function pointers for the IRP major
	//	functions that this FSD is prepared to	handle ...
	//	NT Version 4.0 has 28 possible functions that a
	//	kernel mode driver can handle.
	//	NT Version 3.51 and before has only 22 such functions,
	//	of which 18 are typically interesting to most FSD's.
	
	//	The only interesting new functions that a FSD might
	//	want to respond to beginning with Version 4.0 are the
	//	IRP_MJ_QUERY_QUOTA and the IRP_MJ_SET_QUOTA requests.
	
	//	The code below does not handle quota manipulation, neither
	//	does the NT Version 4.0 operating system (or I/O Manager).
	//	However, you should be on the lookout for any such new
	//	functionality that your FSD might have to implement in
	//	the near future.
	
	DriverObject->MajorFunction[IRP_MJ_CREATE]					= Ext2Create;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]					= Ext2Close;
	DriverObject->MajorFunction[IRP_MJ_READ]					= Ext2Read;
	DriverObject->MajorFunction[IRP_MJ_WRITE]					= Ext2Write;

	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]		= Ext2FileInfo;
	DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]			= Ext2FileInfo;

	DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]			= Ext2Flush;
	// To implement support for querying and modifying volume attributes
	// (volume information query/set operations), enable initialization
	// of the following two function pointers and then implement the supporting
	// functions. Use Chapter 11 in the text to assist you in your efforts.
	DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = Ext2QueryVolInfo;
	DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = Ext2SetVolInfo;
	DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]	= Ext2DirControl;
	// To implement support for file system IOCTL calls, enable initialization
	// of the following function pointer and implement appropriate support. Use
	// Chapter 11 in the text to assist you in your efforts.
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = Ext2FileSystemControl;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]		= Ext2DeviceControl;
	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]				= Ext2Shutdown;
	// For byte-range lock support, enable initialization of the following
	// function pointer and implement appropriate support. Use Chapter 10
	// in the text to assist you in your efforts.
	// DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]		= Ext2LockControl;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP]				= Ext2Cleanup;
	// If your FSD supports security attributes, you should provide appropriate
	// dispatch entry points and initialize the function pointers as given below.
	// DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]		= Ext2Security;
	// DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]		= Ext2Security;
	// If you support extended attributes, you should provide appropriate
	// dispatch entry points and initialize the function pointers as given below.
	// DriverObject->MajorFunction[IRP_MJ_QUERY_EA]				= Ext2ExtendedAttr;
	// DriverObject->MajorFunction[IRP_MJ_SET_EA]				= Ext2ExtendedAttr;

	// Now, it is time to initialize the fast-io stuff ...
/*
	DriverObject->FastIoDispatch = NULL;

*/
	PtrFastIoDispatch = DriverObject->FastIoDispatch = &(Ext2GlobalData.Ext2FastIoDispatch);
	
	// initialize the global fast-io structure
	//	NOTE: The fast-io structure has undergone a substantial revision
	//	in Windows NT Version 4.0. The structure has been extensively expanded.
	//	Therefore, if your driver needs to work on both V3.51 and V4.0+,
	//	you will have to be able to distinguish between the two versions at compile time.
	PtrFastIoDispatch->SizeOfFastIoDispatch	= sizeof(FAST_IO_DISPATCH);
	PtrFastIoDispatch->FastIoCheckIfPossible	= Ext2FastIoCheckIfPossible;
	PtrFastIoDispatch->FastIoRead				= Ext2FastIoRead;
	PtrFastIoDispatch->FastIoWrite				= Ext2FastIoWrite;
	PtrFastIoDispatch->FastIoQueryBasicInfo		= Ext2FastIoQueryBasicInfo;
	PtrFastIoDispatch->FastIoQueryStandardInfo	= Ext2FastIoQueryStdInfo;
	PtrFastIoDispatch->FastIoLock				= Ext2FastIoLock;
	PtrFastIoDispatch->FastIoUnlockSingle		= Ext2FastIoUnlockSingle;
	PtrFastIoDispatch->FastIoUnlockAll			= Ext2FastIoUnlockAll;
	PtrFastIoDispatch->FastIoUnlockAllByKey		= Ext2FastIoUnlockAllByKey;
	PtrFastIoDispatch->AcquireFileForNtCreateSection = Ext2FastIoAcqCreateSec;
	PtrFastIoDispatch->ReleaseFileForNtCreateSection = Ext2FastIoRelCreateSec;

	// the remaining are only valid under NT Version 4.0 and later
#if(_WIN32_WINNT >= 0x0400)
	PtrFastIoDispatch->FastIoQueryNetworkOpenInfo	= Ext2FastIoQueryNetInfo;
	PtrFastIoDispatch->AcquireForModWrite		= Ext2FastIoAcqModWrite;
	PtrFastIoDispatch->ReleaseForModWrite		= Ext2FastIoRelModWrite;
	PtrFastIoDispatch->AcquireForCcFlush		= Ext2FastIoAcqCcFlush;
	PtrFastIoDispatch->ReleaseForCcFlush		= Ext2FastIoRelCcFlush;

	// MDL functionality
	PtrFastIoDispatch->MdlRead					= Ext2FastIoMdlRead;
	PtrFastIoDispatch->MdlReadComplete			= Ext2FastIoMdlReadComplete;
	PtrFastIoDispatch->PrepareMdlWrite			= Ext2FastIoPrepareMdlWrite;
	PtrFastIoDispatch->MdlWriteComplete			= Ext2FastIoMdlWriteComplete;

	// although this FSD does not support compressed read/write functionality,
	//	NTFS does, and if you design a FSD that can provide such functionality,
	//	you should consider initializing the fast io entry points for reading
	//	and/or writing compressed data ...
#endif	// (_WIN32_WINNT >= 0x0400)


	// last but not least, initialize the Cache Manager callback functions
	//	which are used in CcInitializeCacheMap()
	Ext2GlobalData.CacheMgrCallBacks.AcquireForLazyWrite = Ext2AcqLazyWrite;
	Ext2GlobalData.CacheMgrCallBacks.ReleaseFromLazyWrite = Ext2RelLazyWrite;
	Ext2GlobalData.CacheMgrCallBacks.AcquireForReadAhead = Ext2AcqReadAhead;
	Ext2GlobalData.CacheMgrCallBacks.ReleaseFromReadAhead = Ext2RelReadAhead;

	return;
}


VOID NTAPI Ext2QueueHandlerThread( 
	IN PVOID StartContext )
{		
		
	DebugTrace(DEBUG_TRACE_MISC,   "Ext2QueueHandlerThread!!!", 0);
		
	while( 1 )
	{	
		KeWaitForSingleObject( &Ext2GlobalData.ThreadQueue.QueueEvent,
			Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );
		
		DebugTrace(DEBUG_TRACE_MISC,   "Ext2QueueHandlerThread Alerted!!!", 0);
		
		while( !IsListEmpty( &Ext2GlobalData.ThreadQueue.ThreadQueueListHead ) )
		{
			HANDLE				ThreadHandle;
			PLIST_ENTRY			PtrEntry = NULL;
			PtrExt2IrpContext	PtrIrpContext = NULL;


			PtrEntry = ExInterlockedRemoveHeadList( 
				&Ext2GlobalData.ThreadQueue.ThreadQueueListHead, 
				&Ext2GlobalData.ThreadQueue.SpinLock );
			ASSERT( PtrEntry );
			PtrIrpContext = CONTAINING_RECORD( PtrEntry, Ext2IrpContext, ThreadQueueListEntry );
			
			PsCreateSystemThread( 
				&ThreadHandle, (ACCESS_MASK) 0L, 
				NULL, NULL, NULL, Ext2CommonDispatch, PtrIrpContext );
		}
	}	
}

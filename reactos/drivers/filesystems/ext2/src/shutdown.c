/*************************************************************************
*
* File: shutdown.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the "shutdown notification" dispatch entry point.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_SHUTDOWN
#define			DEBUG_LEVEL						(DEBUG_TRACE_SHUTDOWN)


/*************************************************************************
*
* Function: Ext2Shutdown()
*
* Description:
*	All disk-based FSDs can expect to receive this shutdown notification
*	request whenever the system is about to be halted gracefully. If you
*	design and implement a network redirector, you must register explicitly
*	for shutdown notification by invoking the IoRegisterShutdownNotification()
*	routine from your driver entry.
*
*	Note that drivers that register to receive shutdown notification get
*	invoked BEFORE disk-based FSDs are told about the shutdown notification.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant.
*
*************************************************************************/
NTSTATUS NTAPI Ext2Shutdown(
	PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
	PIRP				Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
	PtrExt2IrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DebugTrace(DEBUG_TRACE_IRP_ENTRY, "Shutdown IRP received...", 0);

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

		RC = Ext2CommonShutdown(PtrIrpContext, Irp);

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
* Function: Ext2CommonShutdown()
*
* Description:
*	The actual work is performed here. Basically, all we do here is
*	internally invoke a flush on all mounted logical volumes. This, in
*	tuen, will result in all open file streams being flushed to disk.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant
*
*************************************************************************/
NTSTATUS NTAPI Ext2CommonShutdown(
PtrExt2IrpContext			PtrIrpContext,
PIRP							PtrIrp)
{
	NTSTATUS					RC = STATUS_SUCCESS;
	PIO_STACK_LOCATION			PtrIoStackLocation = NULL;

	try 
	{
		// First, get a pointer to the current I/O stack location
		PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
		ASSERT(PtrIoStackLocation);

		// (a) Block all new "mount volume" requests by acquiring an appropriate
		//		 global resource/lock.
		// (b) Go through your linked list of mounted logical volumes and for
		//		 each such volume, do the following:
		//		 (i) acquire the volume resource exclusively
		//		 (ii) invoke Ext2FlushLogicalVolume() (internally) to flush the
		//				open data streams belonging to the volume from the system
		//				cache
		//		 (iii) Invoke the physical/virtual/logical target device object
		//				on which the volume is mounted and inform this device
		//				about the shutdown request (Use IoBuildSynchronousFsdRequest()
		//				to create an IRP with MajorFunction = IRP_MJ_SHUTDOWN that you
		//				will then issue to the target device object).
		//		 (iv) Wait for the completion of the shutdown processing by the target
		//				device object
		//		 (v) Release the VCB resource you will have acquired in (i) above.

		// Once you have processed all the mounted logical volumes, you can release
		// all acquired global resources and leave (in peace :-)

	

/*////////////////////////////////////////////
		//
		//	Update the Group...
		//
		if( PtrVCB->LogBlockSize )
		{
			//	First block contains the descriptors...
			VolumeByteOffset.QuadPart = LogicalBlockSize;
		}
		else
		{
			//	Second block contains the descriptors...
			VolumeByteOffset.QuadPart = LogicalBlockSize * 2;
		}

		NumberOfBytesToRead = sizeof( struct ext2_group_desc );
		NumberOfBytesToRead = Ext2Align( NumberOfBytesToRead, LogicalBlockSize );

		if (!CcMapData( PtrVCB->PtrStreamFileObject,
               &VolumeByteOffset,
               NumberOfBytesToRead,
               TRUE,
               &PtrBCB,
               &PtrCacheBuffer )) 
		{
			DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
			try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
		}
		else
		{
			//	
			//	Saving up Often Used Group Descriptor Information in the VCB...
			//
			unsigned int DescIndex ;

			DebugTrace(DEBUG_TRACE_MISC,   "Cache hit while reading in volume meta data", 0);
			PtrGroupDescriptor = (PEXT2_GROUP_DESCRIPTOR )PtrCacheBuffer;
			for( DescIndex = 0; DescIndex < PtrVCB->NoOfGroups; DescIndex++ )
			{
				PtrVCB->PtrGroupDescriptors[ DescIndex ].InodeTablesBlock 
					= PtrGroupDescriptor[ DescIndex ].bg_inode_table;

				PtrVCB->PtrGroupDescriptors[ DescIndex ].InodeBitmapBlock 
					= PtrGroupDescriptor[ DescIndex ].bg_inode_bitmap
					;
				PtrVCB->PtrGroupDescriptors[ DescIndex ].BlockBitmapBlock 
					= PtrGroupDescriptor[ DescIndex ].bg_block_bitmap
					;
				PtrVCB->PtrGroupDescriptors[ DescIndex ].FreeBlocksCount 
					= PtrGroupDescriptor[ DescIndex ].bg_free_blocks_count;

				PtrVCB->PtrGroupDescriptors[ DescIndex ].FreeInodesCount 
					= PtrGroupDescriptor[ DescIndex ].bg_free_inodes_count;
			}
			CcUnpinData( PtrBCB );
			PtrBCB = NULL;
		}
*/////////////////////////////////////////////

	} 
	finally 
	{

		// See the read/write examples for how to fill in this portion

	} // end of "finally" processing

	return(RC);
}

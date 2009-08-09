/*************************************************************************
*
* File: DiskIO.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Should contain code to handle Disk IO.
*
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

#define			EXT2_BUG_CHECK_ID				EXT2_FILE_DISK_IO

#define			DEBUG_LEVEL						( DEBUG_TRACE_DISKIO )


/*************************************************************************
*
* Function: Ext2ReadLogicalBlocks()
*
* Description:
*	The higherlevel functions will use this to read in logical blocks
*	This function deals with the logical to physical block translation
*
*	LogicalBlock -	This is a 1 based index.
*					That is, the first block is Block 1
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL 
*
* Return Value: The Status of the Read IO
*
*************************************************************************/
NTSTATUS NTAPI Ext2ReadLogicalBlocks (
	PDEVICE_OBJECT		PtrTargetDeviceObject,	//	the Target Device Object
	VOID				*Buffer,				//	The Buffer that takes the data read in
	LARGE_INTEGER		StartLogicalBlock,		//	The logical block from which reading is to start
	unsigned int		NoOfLogicalBlocks		//	The no. of logical blocks to be read
	)					
{
	//	The Status to be returned...
	NTSTATUS Status = STATUS_SUCCESS;

	//	The Device Object representing the mounted volume
	PDEVICE_OBJECT PtrVolumeDeviceObject = NULL;
	
	//	Volume Control Block
	PtrExt2VCB PtrVCB = NULL;

	//	Logical Block Size
	ULONG LogicalBlockSize;

	// Physical Block Size
	ULONG PhysicalBlockSize;

	//	The starting Physical Block No...
	LARGE_INTEGER StartPhysicalBlock;

	unsigned int		NoOfPhysicalBlocks;



	//	Done with declerations...
	//	Now for some code ;)

	//	Getting the Logical and Physical Sector sizes
	PtrVolumeDeviceObject = PtrTargetDeviceObject->Vpb->DeviceObject;
	ASSERT( PtrVolumeDeviceObject );
	PtrVCB = (PtrExt2VCB)(PtrVolumeDeviceObject->DeviceExtension);
	ASSERT(PtrVCB);
	ASSERT(PtrVCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);

	LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
	PhysicalBlockSize = PtrTargetDeviceObject->SectorSize;
	
	NoOfPhysicalBlocks = NoOfLogicalBlocks * LogicalBlockSize / PhysicalBlockSize;

	StartPhysicalBlock.QuadPart = ( StartLogicalBlock.QuadPart ) * 
									( LogicalBlockSize / PhysicalBlockSize );

	Status = Ext2ReadPhysicalBlocks( PtrTargetDeviceObject,
					Buffer,	StartPhysicalBlock,	NoOfPhysicalBlocks );
		
	return Status;
}

/*************************************************************************
*
* Function: Ext2ReadPhysicalBlocks()
*
* Description:
*	The higherlevel functions will use this to read in physical blocks
*
*	PhysicalBlock -	This is a 0 based number.
*					That is, the first block is Block 0
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL 
*
* Return Value: The Status of the Read IO
*
*************************************************************************/
NTSTATUS NTAPI Ext2ReadPhysicalBlocks (
	PDEVICE_OBJECT		PtrTargetDeviceObject,	//	the Target Device Object
	VOID				*Buffer,				//	The Buffer that takes the data read in
	LARGE_INTEGER		StartPhysicalBlock,		//	The block from which reading is to start
	unsigned int		NoOfBlocks		//	The no. of blocks to be read
	)					
{
	//	The Status to be returned...
	NTSTATUS Status = STATUS_SUCCESS;

	// Physical Block Size
	ULONG PhysicalBlockSize;

	// No of bytes to read
	ULONG NumberOfBytesToRead;

	//	Synchronisation Event
	KEVENT Event;

	//	IRP
    PIRP Irp;

	//	Status Block
    IO_STATUS_BLOCK Iosb;

	//	Byte Offset
	LARGE_INTEGER ByteOffset;

	//	Done with declerations...
	//	Now for some code ;)

	//	Getting the Physical Sector size
	PhysicalBlockSize = PtrTargetDeviceObject->SectorSize;
	
	NumberOfBytesToRead = PhysicalBlockSize * NoOfBlocks;

	ByteOffset.QuadPart = StartPhysicalBlock.QuadPart * PhysicalBlockSize;

	try
	{
		//
		//  Initialize the event we're going to use
		//
		KeInitializeEvent( &Event, NotificationEvent, FALSE );

		//
		//  Build the irp for the operation and also set the overrride flag
		//
		Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
											PtrTargetDeviceObject,
											Buffer,
											NumberOfBytesToRead,
											&ByteOffset ,
											&Event,
											&Iosb );

		if ( Irp == NULL ) 
		{
			DebugTrace(DEBUG_TRACE_MISC,   " !!!! Unable to create an IRP", 0 );
			Status = STATUS_INSUFFICIENT_RESOURCES;
			try_return();
		}

		//
		//  Call the device to do the read and wait for it to finish.
		//
		Status = IoCallDriver( PtrTargetDeviceObject, Irp );

		//
		//	Check if it is done already!!!!
		//
		if (Status == STATUS_PENDING) 
		{
			//
			//	Read not done yet...
			//	Wait till it is...
			//
			(VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );
			Status = Iosb.Status;
		}

		try_exit:	NOTHING;
	}
	finally
	{
		if (!NT_SUCCESS(Status)) 
		{
			if( Status == STATUS_VERIFY_REQUIRED )
			{
				DebugTrace(DEBUG_TRACE_MISC,   " !!!! Verify Required! Failed to read disk",0 );
			}
			else if (Status == STATUS_INVALID_PARAMETER) 
			{
				DebugTrace(DEBUG_TRACE_MISC,   " !!!! Invalid Parameter! Failed to read disk",0 );
			}
			else 
			{
				DebugTrace(DEBUG_TRACE_MISC,   " !!!! Failed to read disk! Status returned = %d", Status );
			}
		}
	}
	return Status;
}



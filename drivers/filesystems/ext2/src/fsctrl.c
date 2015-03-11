/*************************************************************************
*
* File: fsctrl.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the various File System Control calls.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/



#include "ext2fsd.h"



// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_FILE_CONTROL
#define			DEBUG_LEVEL						(DEBUG_TRACE_FSCTRL)


NTSTATUS
Ext2MountVolume(
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp );

NTSTATUS
Ext2GetPartitionInfo(
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PPARTITION_INFORMATION PartitionInformation
    );

NTSTATUS
Ext2GetDriveLayout(
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayoutInformation,
	IN int BufferSize
    );

BOOLEAN
Ext2PerformVerifyDiskRead(
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID Buffer,
    IN LONGLONG Lbo,
    IN ULONG NumberOfBytesToRead
    );

NTSTATUS Ext2UserFileSystemRequest( 
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp );


/*************************************************************************
*
* Function: Ext2FileSystemControl
*
* Description:
*	The I/O Manager will invoke this routine to handle a 
*   File System Control IRP
*
* Expected Interrupt Level (for execution) :
*  
*	???
*
* Arguments:
*
*    DeviceObject - Supplies the volume device object where the
*                   file exists
*
*    Irp - Supplies the Irp being processed
*
*
* Return Value:
*
*    NTSTATUS - The FSD status for the IRP
*
*************************************************************************/
NTSTATUS NTAPI
Ext2FileSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
  
    NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION IrpSp;
    
	DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "File System Control IRP Received...", 0);
		
	//	Ext2BreakPoint();

	FsRtlEnterFileSystem();

	ASSERT(DeviceObject);
	ASSERT(Irp);

	//
    //  Get a pointer to the current Irp stack location
    //
    IrpSp = IoGetCurrentIrpStackLocation( Irp );
	

	if( IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME )
	{
		DebugTrace(DEBUG_TRACE_MOUNT,   "Mount Request Received...", 0);
		Status = Ext2MountVolume ( Irp, IrpSp );
		Ext2CompleteRequest( Irp, Status );
	}
	else if( IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST )
	{
		DebugTrace(DEBUG_TRACE_FSCTRL,   "IRP_MN_USER_FS_REQUEST received...", 0);
		Status = Ext2UserFileSystemRequest( Irp, IrpSp );
		Ext2CompleteRequest( Irp, Status );
	}
	else 
	{
		if( IrpSp->MinorFunction == IRP_MN_VERIFY_VOLUME )
		{
			DebugTrace(DEBUG_TRACE_FSCTRL,   "IRP_MN_VERIFY_VOLUME received...", 0);
		}
		else if( IrpSp->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM )
		{
			DebugTrace(DEBUG_TRACE_FSCTRL,   "IRP_MN_LOAD_FILE_SYSTEM received...", 0);
		}
		else
		{
			DebugTrace(DEBUG_TRACE_FSCTRL,   "Unknown Minor IRP code received...", 0);
		}

		Status = STATUS_INVALID_DEVICE_REQUEST;
		Ext2CompleteRequest( Irp, Status );
	}
	
	FsRtlExitFileSystem();

    return Status;
}



/*************************************************************************
*
* Function: Ext2MountVolume()
*
* Description:
*	This routine verifies and mounts the volume; 
*	Called by FSCTRL IRP handler to attempt a 
*		volume mount.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL 
*
*
* Arguments:
*
*    Irp - Supplies the Irp being processed
*	 IrpSp - Irp Stack Location pointer
*
* Return Value: 
*
*   NTSTATUS - The Mount status
*
*************************************************************************/
NTSTATUS
Ext2MountVolume ( 
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp )
{
	
	//	Volume Parameter Block
	PVPB PtrVPB;

	//	The target device object
	PDEVICE_OBJECT TargetDeviceObject = NULL;

	// The new volume device object (to be created if partition is Ext2)
	PDEVICE_OBJECT PtrVolumeDeviceObject = NULL;
	
	//	Return Status
	NTSTATUS Status = STATUS_UNRECOGNIZED_VOLUME;

	// Number of bytes to read for Volume verification...
	unsigned long NumberOfBytesToRead = 0;
	
	//	Starting Offset for 'read'
	LONGLONG StartingOffset = 0;

	//	Boot Sector information...
	PPACKED_BOOT_SECTOR BootSector = NULL;
	
	//	Ext2 Super Block information...
	PEXT2_SUPER_BLOCK SuperBlock = NULL;

	//	Volume Control Block
	PtrExt2VCB			PtrVCB = NULL;

	//	The File Object for the root directory
	PFILE_OBJECT PtrRootFileObject = NULL;
	
	//	Flag
	int WeClearedVerifyRequiredBit;
	
	//	Used by a for loop...
	unsigned int i;
	
	//	
	LARGE_INTEGER VolumeByteOffset;

	unsigned long LogicalBlockSize = 0;

	//	Buffer Control Block
	PBCB PtrBCB = NULL;
			
	//	Cache Buffer - used for pinned access of volume...
	PVOID PtrCacheBuffer = NULL;
	
	PEXT2_GROUP_DESCRIPTOR	PtrGroupDescriptor = NULL;

	// Inititalising variables
	
	PtrVPB = IrpSp->Parameters.MountVolume.Vpb;
	TargetDeviceObject = IrpSp->Parameters.MountVolume.DeviceObject;
	
	try
	{
		//
		//	1. Reading in Volume meta data
		//

		//	Temporarily clear the DO_VERIFY_VOLUME Flag
		WeClearedVerifyRequiredBit = 0;
		if ( Ext2IsFlagOn( PtrVPB->RealDevice->Flags, DO_VERIFY_VOLUME ) ) 
		{
            Ext2ClearFlag( PtrVPB->RealDevice->Flags, DO_VERIFY_VOLUME );
            WeClearedVerifyRequiredBit = 1;
        }

		//	Allocating memory for reading in Boot Sector...
		NumberOfBytesToRead = Ext2Align( sizeof( EXT2_SUPER_BLOCK ), TargetDeviceObject->SectorSize );
		BootSector = Ext2AllocatePool( PagedPool, NumberOfBytesToRead  );
		RtlZeroMemory( BootSector, NumberOfBytesToRead );

		//	Reading in Boot Sector
		StartingOffset = 0L;
		Ext2PerformVerifyDiskRead ( TargetDeviceObject,
			BootSector, StartingOffset, NumberOfBytesToRead );

		// Reject a volume that contains fat artifacts
		
		DebugTrace(DEBUG_TRACE_MOUNT, "OEM[%s]", BootSector->Oem);
		if (BootSector->Oem[0])
		{
		    try_return();
		}

		//	Allocating memory for reading in Super Block...
		
		SuperBlock = Ext2AllocatePool( PagedPool, NumberOfBytesToRead  );
		RtlZeroMemory( SuperBlock, NumberOfBytesToRead );
		StartingOffset = 1024;

		//	Reading in the Super Block...
		Ext2PerformVerifyDiskRead ( TargetDeviceObject,
			SuperBlock, StartingOffset, NumberOfBytesToRead );

		//	Resetting the DO_VERIFY_VOLUME Flag
		if( WeClearedVerifyRequiredBit ) 
		{
			PtrVPB->RealDevice->Flags |= DO_VERIFY_VOLUME;
		}

		// Verifying the Super Block..
		if( SuperBlock->s_magic == EXT2_SUPER_MAGIC )
		{
			//
			//	Found a valid super block.
			//	No more tests for now.
			//	Assuming that this is an ext2 partition...
			//	Going ahead with mount.
			//
			DebugTrace(DEBUG_TRACE_MOUNT,   "Valid Ext2 partition detected\nMounting %s...", SuperBlock->s_volume_name);
			//
			//	2. Creating a volume device object
			//
	        if (!NT_SUCCESS( IoCreateDevice( 
					Ext2GlobalData.Ext2DriverObject,	//	(This) Driver object
					Ext2QuadAlign( sizeof(Ext2VCB) ),	//	Device Extension
                    NULL,								//	Device Name - no name ;)
                    FILE_DEVICE_DISK_FILE_SYSTEM,		//	Disk File System
                    0,									//	DeviceCharacteristics
                    FALSE,								//	Not an exclusive device
                    (PDEVICE_OBJECT *)&PtrVolumeDeviceObject)) //	The Volume Device Object
					) 
			{
	            try_return();
			}

			//	
			//  Our alignment requirement is the larger of the processor alignment requirement
			//  already in the volume device object and that in the TargetDeviceObject
			//

			if (TargetDeviceObject->AlignmentRequirement > PtrVolumeDeviceObject->AlignmentRequirement) 
			{
				PtrVolumeDeviceObject->AlignmentRequirement = TargetDeviceObject->AlignmentRequirement;
			}
			
			//
			//	Clearing the Device Initialising Flag
			//
			Ext2ClearFlag( PtrVolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);


			//
			//	Setting the Stack Size for the newly created Volume Device Object
			//
			PtrVolumeDeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);
			

			//
			//	3. Creating the link between Target Device Object 
			//	and the Volume Device Object via the Volume Parameter Block
			//
			PtrVPB->DeviceObject = PtrVolumeDeviceObject;
			
			//	Remembring the Volume parameters in the VPB bock
			for( i = 0; i < 16 ; i++ )
			{
				PtrVPB->VolumeLabel[i] = SuperBlock->s_volume_name[i];
				if( SuperBlock->s_volume_name[i] == 0 )
					break;
			}
			PtrVPB->VolumeLabelLength = i * 2;
			PtrVPB->SerialNumber = ((ULONG*)SuperBlock->s_uuid)[0];
				
			//
			//	4. Initialise the Volume Comtrol Block
			//
			{
				LARGE_INTEGER AllocationSize;

				AllocationSize .QuadPart = 
					( EXT2_MIN_BLOCK_SIZE << SuperBlock->s_log_block_size ) * 
					SuperBlock->s_blocks_count;

				Ext2InitializeVCB(
					PtrVolumeDeviceObject, 
					TargetDeviceObject,
					PtrVPB,
					&AllocationSize);
				PtrVCB = (PtrExt2VCB)(PtrVolumeDeviceObject->DeviceExtension);
				ASSERT( PtrVCB );
			}

			PtrVCB->InodesCount = SuperBlock->s_inodes_count;
			PtrVCB->BlocksCount = SuperBlock->s_blocks_count;
			PtrVCB->ReservedBlocksCount = SuperBlock->s_r_blocks_count;
			PtrVCB->FreeBlocksCount = SuperBlock->s_free_blocks_count;
			PtrVCB->FreeInodesCount = SuperBlock->s_free_inodes_count;
			PtrVCB->LogBlockSize = SuperBlock->s_log_block_size;
			PtrVCB->InodesPerGroup = SuperBlock->s_inodes_per_group;
			PtrVCB->BlocksPerGroup = SuperBlock->s_blocks_per_group;
			PtrVCB->NoOfGroups = ( SuperBlock->s_blocks_count - SuperBlock->s_first_data_block 
								+ SuperBlock->s_blocks_per_group - 1 ) 
								/ SuperBlock->s_blocks_per_group;
			if( SuperBlock->s_rev_level )
			{
				PtrVCB->InodeSize = SuperBlock->s_inode_size;
			}
			else
			{
				PtrVCB->InodeSize = sizeof( EXT2_INODE );
			}

			PtrVCB->PtrGroupDescriptors = Ext2AllocatePool( NonPagedPool, sizeof( Ext2GroupDescriptors ) * PtrVCB->NoOfGroups  );
			
			RtlZeroMemory( PtrVCB->PtrGroupDescriptors , sizeof( Ext2GroupDescriptors ) * PtrVCB->NoOfGroups );

			//
			//	Attempting to Read in some matadata from the Cache...
			//	using pin access...
			//
			LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
	
			//
			//	Reading Group Descriptors...
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

			NumberOfBytesToRead = PtrVCB->NoOfGroups * sizeof( struct ext2_group_desc );
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

			//
			//	5. Creating a Root Directory FCB
			//
			PtrRootFileObject = IoCreateStreamFileObject(NULL, TargetDeviceObject );
			if( !PtrRootFileObject )
			{
				try_return();
			}
			//
			//	Associate the file stream with the Volume parameter block...
			//	I do it now
			//
			PtrRootFileObject->Vpb = PtrVCB->PtrVPB;

			PtrRootFileObject->ReadAccess = TRUE;
			PtrRootFileObject->WriteAccess = TRUE;

			{
				PtrExt2ObjectName		PtrObjectName;
				LARGE_INTEGER ZeroSize;

				PtrObjectName = Ext2AllocateObjectName();
				RtlInitUnicodeString( &PtrObjectName->ObjectName, L"\\" );
				Ext2CopyWideCharToUnicodeString( &PtrObjectName->ObjectName, L"\\" );


				ZeroSize.QuadPart = 0;
				if ( !NT_SUCCESS( Ext2CreateNewFCB( 
						&PtrVCB->PtrRootDirectoryFCB,	//	Root FCB
						ZeroSize,			//	AllocationSize,
						ZeroSize,			//	EndOfFile,
						PtrRootFileObject,		//	The Root Dircetory File Object
						PtrVCB,
						PtrObjectName  )  )  )
				{
					try_return();
				}
				

				PtrVCB->PtrRootDirectoryFCB->FCBFlags |= EXT2_FCB_DIRECTORY | EXT2_FCB_ROOT_DIRECTORY;


			}

			PtrVCB->PtrRootDirectoryFCB->DcbFcb.Dcb.PtrDirFileObject = PtrRootFileObject;
			PtrVCB->PtrRootDirectoryFCB->INodeNo = EXT2_ROOT_INO;
			PtrRootFileObject->SectionObjectPointer = &(PtrVCB->PtrRootDirectoryFCB->NTRequiredFCB.SectionObject);
			RtlInitUnicodeString( &PtrRootFileObject->FileName, L"\\" );

			Ext2InitializeFCBInodeInfo( PtrVCB->PtrRootDirectoryFCB );

			//	
			//	Initiating caching for root directory...
			//

			CcInitializeCacheMap(PtrRootFileObject, 
				(PCC_FILE_SIZES)(&(PtrVCB->PtrRootDirectoryFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize)),
				TRUE,		// We will utilize pin access for directories
				&(Ext2GlobalData.CacheMgrCallBacks), // callbacks
				PtrVCB->PtrRootDirectoryFCB );		// The context used in callbacks


			//
			//	6. Update the VCB Flags
			//
			PtrVCB->VCBFlags |= EXT2_VCB_FLAGS_VOLUME_MOUNTED ;	//	| EXT2_VCB_FLAGS_VOLUME_READ_ONLY;


			//
			//	7. Mount Success
			//
			Status = STATUS_SUCCESS;
			
			{
				//
				//	This block is for testing....
				//	To be removed...

				/*
				EXT2_INODE	Inode ;
				Ext2ReadInode( PtrVCB, 100, &Inode );
				DebugTrace( DEBUG_TRACE_MISC, "Inode size= %lX [FS Ctrl]", Inode.i_size );
				Ext2DeallocInode( NULL, PtrVCB, 0xfb6 );
				*/
			}
			
			//	 ObDereferenceObject( TargetDeviceObject );
		}
		else
		{
			DebugTrace(DEBUG_TRACE_MOUNT,   "Failing mount. Partition not Ext2...", 0);
		}

		try_exit: NOTHING;
	}
	finally 
	{
		//	Freeing Allocated Memory...
		if( SuperBlock != NULL )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [FS Ctrl]", SuperBlock );
			ExFreePool( SuperBlock );
		}
		if( BootSector != NULL )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [FS Ctrl]", BootSector);
			ExFreePool( BootSector );
		}

		// start unwinding if we were unsuccessful
		if (!NT_SUCCESS( Status )) 
		{
						
		}
	}
	
	return Status;
}

/*************************************************************************
*
* Function: Ext2MountVolume()
*
* Description:
*	This routine is used for querying the partition information.
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Arguments:
*
*	TargetDeviceObject - The target of the query
*	PartitionInformation - Receives the result of the query
*
* Return Value:
*
*	NTSTATUS - The return status for the operation
*
*************************************************************************/
NTSTATUS
Ext2GetPartitionInfo (
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PPARTITION_INFORMATION PartitionInformation
    )
{
    PIRP Irp;
    KEVENT *PtrEvent = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    //  Query the partition table
    //
	PtrEvent = ( KEVENT * )Ext2AllocatePool( NonPagedPool, Ext2QuadAlign( sizeof( KEVENT ) )  );
	



    KeInitializeEvent( PtrEvent, NotificationEvent, FALSE );
	
    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_PARTITION_INFO,
                                         TargetDeviceObject,
                                         NULL,
                                         0,
                                         PartitionInformation,
                                         sizeof(PARTITION_INFORMATION),
                                         FALSE,
                                         PtrEvent,
                                         &Iosb );

    if ( Irp == NULL ) 
	{
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [FS Ctrl]", PtrEvent);
		ExFreePool( PtrEvent );
		return 0;
    }

    Status = IoCallDriver( TargetDeviceObject, Irp );

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( PtrEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }
	DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [FS Ctrl]", PtrEvent);
	ExFreePool( PtrEvent );
    return Status;
}

/*************************************************************************
*
* Function: Ext2MountVolume()
*
* Description:
*	This routine is used for querying the Drive Layout Information.
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Arguments:
*
*	TargetDeviceObject - The target of the query
*	PartitionInformation - Receives the result of the query
*
* Return Value:
*
*	NTSTATUS - The return status for the operation
*
*************************************************************************/
NTSTATUS Ext2GetDriveLayout (
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayoutInformation,
	IN int BufferSize
    )
{
    PIRP Irp;
    KEVENT *PtrEvent = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    //  Query the partition table
    //
	PtrEvent = ( KEVENT * )Ext2AllocatePool( NonPagedPool, Ext2QuadAlign( sizeof( KEVENT ) )  );
    KeInitializeEvent( PtrEvent, NotificationEvent, FALSE );
    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_LAYOUT,
                                         TargetDeviceObject,
                                         NULL,
                                         0,
                                         DriveLayoutInformation,
                                         BufferSize,
                                         FALSE,
                                         PtrEvent,
                                         &Iosb );

    if ( Irp == NULL ) 
	{
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [FS Ctrl]", PtrEvent);
		ExFreePool( PtrEvent );
		return 0;
    }

    Status = IoCallDriver( TargetDeviceObject, Irp );

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( PtrEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }
	DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [FS Ctrl]", PtrEvent);
	ExFreePool( PtrEvent );
    return Status;
}


/*************************************************************************
*
* Function: Ext2MountVolume()
*
* Description:
*	This routine is used for performing a verify read...
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Arguments:
*	TargetDeviceObject - The target of the query
*	PartitionInformation - Receives the result of the query
*
* Return Value:
*	NTSTATUS - The return status for the operation
*
*************************************************************************/
BOOLEAN Ext2PerformVerifyDiskRead(
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID Buffer,
    IN LONGLONG Lbo,
    IN ULONG NumberOfBytesToRead )
{
    KEVENT Event;
    PIRP Irp;
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    //  Initialize the event we're going to use
    //
    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the irp for the operation
    //
    ByteOffset.QuadPart = Lbo;
    Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        TargetDeviceObject,
                                        Buffer,
                                        NumberOfBytesToRead,
                                        &ByteOffset,
                                        &Event,
                                        &Iosb );

    if ( Irp == NULL ) 
	{
        Status = FALSE;
    }

    Ext2SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Call the device to do the read and wait for it to finish.
    //
    Status = IoCallDriver( TargetDeviceObject, Irp );
    if (Status == STATUS_PENDING) 
	{
        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );
        Status = Iosb.Status;
    }

    ASSERT(Status != STATUS_VERIFY_REQUIRED);

    //
    //  Special case this error code because this probably means we used
    //  the wrong sector size and we want to reject STATUS_WRONG_VOLUME.
    //

    if (Status == STATUS_INVALID_PARAMETER) 
	{
        DbgPrint("Ext2PerformVerifyDiskRead Invalid Param\n");
        return FALSE;
    }

    if (Status == STATUS_NO_MEDIA_IN_DEVICE) 
	{
        DebugTrace(DEBUG_TRACE_MOUNT, "NO MEDIA in DEVICE!", 0);
        return FALSE;
    }

    //
    //  If it doesn't succeed then either return or raise the error.
    //

    if (!NT_SUCCESS(Status)) 
	{
	    DbgPrint("Ext2PerformVerifyDiskRead Fail Status %x\n",Status);
            return FALSE;
    }

    //
    //  And return to our caller
    //
    return TRUE;
}

/*************************************************************************
*
* Function: Ext2UserFileSystemRequest()
*
* Description:
*	This routine handles User File System Requests
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL 
*
*
* Arguments:
*
*    Irp - Supplies the Irp being processed
*	 IrpSp - Irp Stack Location pointer
*
* Return Value: NT_STATUS
*
*************************************************************************/
NTSTATUS Ext2UserFileSystemRequest ( 
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp )
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG FsControlCode;

	IrpSp = IoGetCurrentIrpStackLocation( Irp );
	
	try
	{
#ifdef _GNU_NTIFS_
		FsControlCode = ((PEXTENDED_IO_STACK_LOCATION)IrpSp)->Parameters.FileSystemControl.FsControlCode;
#else
		FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;
#endif

		switch ( FsControlCode ) 
		{

		case FSCTL_REQUEST_OPLOCK_LEVEL_1:
			DebugTrace(DEBUG_TRACE_FSCTRL,   "FSCTL_REQUEST_OPLOCK_LEVEL_1", 0);
			break;
		case FSCTL_REQUEST_OPLOCK_LEVEL_2:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL ", 0);
			break;
		case FSCTL_REQUEST_BATCH_OPLOCK:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_REQUEST_OPLOCK_LEVEL_2 ", 0);
			break;
		case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
			DebugTrace(DEBUG_TRACE_MISC,   " FSCTL_OPLOCK_BREAK_ACKNOWLEDGE ", 0);
			break;
		case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_OPBATCH_ACK_CLOSE_PENDING ", 0);
			break;
		case FSCTL_OPLOCK_BREAK_NOTIFY:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_OPLOCK_BREAK_NOTIFY ", 0);
			break;
		case FSCTL_OPLOCK_BREAK_ACK_NO_2:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_OPLOCK_BREAK_ACK_NO_2 ", 0);
			break;
		case FSCTL_LOCK_VOLUME:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_LOCK_VOLUME ", 0);
			break;
		case FSCTL_UNLOCK_VOLUME:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_UNLOCK_VOLUME ", 0);
			break;
		case FSCTL_DISMOUNT_VOLUME:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_DISMOUNT_VOLUME ", 0);
			break;
		case FSCTL_MARK_VOLUME_DIRTY:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_MARK_VOLUME_DIRTY ", 0);
			break;
		case FSCTL_IS_VOLUME_DIRTY:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_IS_VOLUME_DIRTY ", 0);
			break;
		case FSCTL_IS_VOLUME_MOUNTED:
			Status = Ext2VerifyVolume(Irp, IrpSp );
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_IS_VOLUME_MOUNTED ", 0);
			break;
		case FSCTL_IS_PATHNAME_VALID:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_IS_PATHNAME_VALID ", 0);
			break;
		case FSCTL_QUERY_RETRIEVAL_POINTERS:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_QUERY_RETRIEVAL_POINTERS ", 0);
			break;
		case FSCTL_QUERY_FAT_BPB:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_QUERY_FAT_BPB ", 0);
			break;
		case FSCTL_FILESYSTEM_GET_STATISTICS:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_FILESYSTEM_GET_STATISTICS ", 0);
			break;
		case FSCTL_GET_VOLUME_BITMAP:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_GET_VOLUME_BITMAP ", 0);
			break;
		case FSCTL_GET_RETRIEVAL_POINTERS:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_GET_RETRIEVAL_POINTERS ", 0);
			break;
		case FSCTL_MOVE_FILE:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_MOVE_FILE ", 0);
			break;
		case FSCTL_ALLOW_EXTENDED_DASD_IO:
			DebugTrace(DEBUG_TRACE_FSCTRL,   " FSCTL_ALLOW_EXTENDED_DASD_IO ", 0);
			break;
		default :
			DebugTrace(DEBUG_TRACE_FSCTRL,   "Unknown FSCTRL !!!", 0);

		}
	}
	finally
	{

	}
	return Status;
}



NTSTATUS NTAPI Ext2VerifyVolume (
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp )
{

	PVPB PtrVPB;
	
	PtrVPB = IrpSp->Parameters.VerifyVolume.Vpb;
	if( IrpSp->FileObject )
	{
		PtrVPB = IrpSp->FileObject->Vpb;
	}
	if( !PtrVPB )
	{
		PtrVPB = IrpSp->Parameters.VerifyVolume.Vpb;
	}

	if( !PtrVPB )
	{
		return STATUS_WRONG_VOLUME;
	}


	if ( Ext2IsFlagOn( PtrVPB->RealDevice->Flags, DO_VERIFY_VOLUME ) ) 
	{
		//
		//	Not doing a verify!
		//	Just acting as if everyting is fine!
		//	THis should do for now
		//
        Ext2ClearFlag( PtrVPB->RealDevice->Flags, DO_VERIFY_VOLUME );
		
	}
	return STATUS_SUCCESS;
}

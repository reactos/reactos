/*************************************************************************
*
* File: volinfo.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains code to handle the various Volume Information related calls.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/



#include "ext2fsd.h"




// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_VOL_INFORMATION
#define			DEBUG_LEVEL						(DEBUG_TRACE_VOLINFO)


/*************************************************************************
*
* Function: Ext2QueryVolInfo()
*
* Description:
*	The I/O Manager will invoke this routine to handle a 
*   Query Volume Info IRP
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
NTSTATUS NTAPI Ext2QueryVolInfo (
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP Irp)
{

	//	The Return Status
    NTSTATUS Status = STATUS_SUCCESS;

	//	The IRP Stack Location
	PIO_STACK_LOCATION	IrpSp = NULL;

	//	Volume Control Block
	PtrExt2VCB			PtrVCB = NULL;
	
	//	The class of the query IRP
    FS_INFORMATION_CLASS FsInformationClass;
	
	//	The System Buffer Pointer
    PVOID Buffer = NULL;
	
	//	Parameter Length
	ULONG Length = 0;
	
	//	Bytes copied...
	ULONG BytesCopied = 0;

	//	Pointers to the Output Information...
	PFILE_FS_VOLUME_INFORMATION		PtrVolumeInformation	= NULL;
	PFILE_FS_SIZE_INFORMATION		PtrSizeInformation		= NULL;
	PFILE_FS_ATTRIBUTE_INFORMATION	PtrAttributeInformation	= NULL;
	PFILE_FS_DEVICE_INFORMATION		PtrDeviceInformation	= NULL;
	PFILE_FS_FULL_SIZE_INFORMATION	PtrFullSizeInformation	= NULL;


	//	Now for the handler code...
    DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "QueryVolumeInformation IRP", 0);

    FsRtlEnterFileSystem();

    try 
	{
		// Getting a pointer to the current I/O stack location
		IrpSp = IoGetCurrentIrpStackLocation(Irp);
		ASSERT( IrpSp );
		
		//	Getting the VCB and Verifying it...
		PtrVCB = ( PtrExt2VCB )( DeviceObject->DeviceExtension );
		ASSERT(PtrVCB);
		ASSERT(PtrVCB->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);
		
		//	Getting the query parameters...
		Length = IrpSp->Parameters.QueryVolume.Length;
		FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
		Buffer = Irp->AssociatedIrp.SystemBuffer;

		//	Now servicing the request depending on the type...
        switch (FsInformationClass) 
		{
        case FileFsVolumeInformation:
			DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - FileFsVolumeInformation", 0);
			PtrVolumeInformation = Buffer;
			PtrVolumeInformation->SupportsObjects = FALSE;
			PtrVolumeInformation->VolumeCreationTime.QuadPart = 0;
			RtlCopyMemory( 
				PtrVolumeInformation->VolumeLabel,	//	destination
				PtrVCB->PtrVPB->VolumeLabel,		//	source
				PtrVCB->PtrVPB->VolumeLabelLength );
			PtrVolumeInformation->VolumeLabelLength = PtrVCB->PtrVPB->VolumeLabelLength;
			PtrVolumeInformation->VolumeSerialNumber = PtrVCB->PtrVPB->SerialNumber;
			BytesCopied = sizeof( FILE_FS_VOLUME_INFORMATION ) + PtrVolumeInformation->VolumeLabelLength - sizeof( WCHAR);
            break;

        case FileFsSizeInformation:
			DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - FileFsSizeInformation", 0);
			PtrSizeInformation = Buffer;
			PtrSizeInformation->BytesPerSector = DeviceObject->SectorSize;
			PtrSizeInformation->AvailableAllocationUnits.QuadPart	= PtrVCB->FreeBlocksCount;
			PtrSizeInformation->SectorsPerAllocationUnit	= ( EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize) / DeviceObject->SectorSize;
			PtrSizeInformation->TotalAllocationUnits.QuadPart		= PtrVCB->BlocksCount;
			BytesCopied = sizeof( FILE_FS_SIZE_INFORMATION );
            break;

        case FileFsDeviceInformation:
            DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - FileFsDeviceInformation", 0);
			PtrDeviceInformation = Buffer;
			PtrDeviceInformation->DeviceType = FILE_DEVICE_DISK;
			PtrDeviceInformation->Characteristics = FILE_DEVICE_IS_MOUNTED;
			BytesCopied = sizeof( FILE_FS_DEVICE_INFORMATION );
            break;

        case FileFsAttributeInformation:
            DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - FileFsAttributeInformation", 0);
			PtrAttributeInformation = Buffer;
			RtlCopyMemory( PtrAttributeInformation->FileSystemName,	L"EXT2", 10 );
			PtrAttributeInformation->FileSystemNameLength = 8;
			PtrAttributeInformation->MaximumComponentNameLength = 255;
			PtrAttributeInformation->FileSystemAttributes  = 
				FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
			BytesCopied = sizeof( FILE_FS_ATTRIBUTE_INFORMATION ) + 8; 

            break;

        case FileFsFullSizeInformation:
            DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - FileFsFullSizeInformation", 0);
			PtrFullSizeInformation = Buffer;
			PtrFullSizeInformation->BytesPerSector = DeviceObject->SectorSize;
			PtrFullSizeInformation->ActualAvailableAllocationUnits.QuadPart	= PtrVCB->FreeBlocksCount;
			PtrFullSizeInformation->SectorsPerAllocationUnit	= (EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize) / DeviceObject->SectorSize;
			PtrFullSizeInformation->TotalAllocationUnits.QuadPart		= PtrVCB->BlocksCount;
			PtrFullSizeInformation->CallerAvailableAllocationUnits.QuadPart = PtrVCB->FreeBlocksCount - PtrVCB->ReservedBlocksCount;
			BytesCopied = sizeof( FILE_FS_FULL_SIZE_INFORMATION );
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
			DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - STATUS_INVALID_PARAMETER", 0);
            break;
        }
		
		if( IrpSp->Parameters.QueryVolume.Length < BytesCopied )	
		{
			BytesCopied = IrpSp->Parameters.QueryVolume.Length;
			Status = STATUS_BUFFER_OVERFLOW;
			DebugTrace(DEBUG_TRACE_MISC,   " === Buffer insufficient", 0);
		}
    }
	finally
	{
		Irp->IoStatus.Information = BytesCopied;
		Ext2CompleteRequest( Irp, Status );
	}
    FsRtlExitFileSystem();

    //
    //  Now return to the caller
    //

    return Status;
}



NTSTATUS NTAPI Ext2SetVolInfo(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP Irp)
{
	//	The Return Status
    NTSTATUS Status = STATUS_SUCCESS;

	//	The IRP Stack Location
	PIO_STACK_LOCATION	IrpSp = NULL;

	//	Volume Control Block
	PtrExt2VCB			PtrVCB = NULL;
	
	//	The class of the query IRP
    FS_INFORMATION_CLASS FsInformationClass;
	
	//	Pointers to the Output Information...
	PFILE_FS_LABEL_INFORMATION		PtrVolumeLabelInformation = NULL;

	//	Now for the handler code...
    DebugTrace(DEBUG_TRACE_IRP_ENTRY,   "Set Volume Information IRP", 0);

    FsRtlEnterFileSystem();

    try 
	{
		// Getting a pointer to the current I/O stack location
		IrpSp = IoGetCurrentIrpStackLocation(Irp);
		ASSERT( IrpSp );
		
		//	Getting the VCB and Verifying it...
		PtrVCB = ( PtrExt2VCB )( DeviceObject->DeviceExtension );
		AssertVCB(PtrVCB);
		
		//	Getting the query parameters...
		//	Length = IrpSp->Parameters.SetVolume.Length;
#ifdef _GNU_NTIFS_
		FsInformationClass = ((PEXTENDED_IO_STACK_LOCATION)IrpSp)->Parameters.SetVolume.FsInformationClass;
#else
		FsInformationClass = IrpSp->Parameters.SetVolume.FsInformationClass;
#endif

		//	Now servicing the request depending on the type...
        switch (FsInformationClass) 
		{
		case FileFsLabelInformation:
			PtrVolumeLabelInformation = Irp->AssociatedIrp.SystemBuffer;
			if( PtrVolumeLabelInformation->VolumeLabelLength > MAXIMUM_VOLUME_LABEL_LENGTH ||	//	This is the maximum that the 
																								//	VPB can take...
				PtrVolumeLabelInformation->VolumeLabelLength > 32 )	//	this is the maximum that Ext2 FS can support..
			{
				try_return( Status = STATUS_INVALID_VOLUME_LABEL );
			}

			PtrVCB->PtrVPB->VolumeLabelLength = (USHORT)PtrVolumeLabelInformation->VolumeLabelLength ;
			RtlCopyMemory( 
				PtrVCB->PtrVPB->VolumeLabel,		//	destination
				PtrVolumeLabelInformation->VolumeLabel,	//	source	
				PtrVolumeLabelInformation->VolumeLabelLength );
			
			{
				//	Now update the volume's super block...

				PEXT2_SUPER_BLOCK	PtrSuperBlock = NULL;
				PBCB				PtrSuperBlockBCB = NULL;
				LARGE_INTEGER		VolumeByteOffset;
				ULONG				LogicalBlockSize = 0;
				ULONG				NumberOfBytesToRead = 0;


				//	Reading in the super block...
				VolumeByteOffset.QuadPart = 1024;

				LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

				//	THis shouldn't be more than a block in size...
				NumberOfBytesToRead = Ext2Align( sizeof( EXT2_SUPER_BLOCK ), LogicalBlockSize );

				if( !CcPinRead( PtrVCB->PtrStreamFileObject,
					   &VolumeByteOffset,
					   NumberOfBytesToRead,
					   TRUE,
					   &PtrSuperBlockBCB,
					   (PVOID*)&PtrSuperBlock ) )
				{
					DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
					try_return( Status = STATUS_INVALID_VOLUME_LABEL );
				}
				else
				{
					ULONG i;
					for( i = 0; i < (PtrVolumeLabelInformation->VolumeLabelLength/2) ; i++ )
					{
						PtrSuperBlock->s_volume_name[i] = 
							(char) PtrVolumeLabelInformation->VolumeLabel[i] ;
						if( PtrSuperBlock->s_volume_name[i] == 0 )
						{
							break;
						}
					}
					if( i < 16 )
					{
						PtrSuperBlock->s_volume_name[i] = 0;
					}


					CcSetDirtyPinnedData( PtrSuperBlockBCB, NULL );
					
					//	Not saving and flushing this information synchronously...
					//	This is not a critical information..
					//	Settling for lazy writing of this information

					//	Ext2SaveBCB( PtrIrpContext, PtrSuperBlockBCB, PtrVCB->PtrStreamFileObject );
					
					if( PtrSuperBlockBCB )
					{
						CcUnpinData( PtrSuperBlockBCB );
						PtrSuperBlockBCB = NULL;
					}
					
				}
			}

			break;
        default:
            Status = STATUS_INVALID_PARAMETER;
			DebugTrace(DEBUG_TRACE_MISC,   "Query Volume - STATUS_INVALID_PARAMETER", 0);
            break;
		}
		
		try_exit: NOTHING;
    }
	finally
	{
		Irp->IoStatus.Information = 0;
		Ext2CompleteRequest( Irp, Status );
	}
    FsRtlExitFileSystem();

    //
    //  Now return to the caller
    //

    return Status;
}

/*************************************************************************
*
* File: metadata.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Should contain code to handle Ext2 Metadata.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

#define			EXT2_BUG_CHECK_ID				EXT2_FILE_METADATA_IO

#define			DEBUG_LEVEL						( DEBUG_TRACE_METADATA )

extern	Ext2Data					Ext2GlobalData;

/*************************************************************************
*
* Function: Ext2ReadInode()
*
* Description:
*
*	The functions will read in the specifiec inode and return it in a buffer
*
* 
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL 
*
*
* Arguements:
*
*
*
* Return Value: The Status of the Read IO
*
*************************************************************************/

NTSTATUS NTAPI Ext2ReadInode (
	PtrExt2VCB		PtrVcb,			//	the Volume Control Block
	uint32			InodeNo,		//	The Inode no
	PEXT2_INODE		PtrInode		//	The Inode Buffer
	)					
{
	//	The Status to be returned...
	NTSTATUS RC = STATUS_SUCCESS;

	//	The Read Buffer Pointer
	BYTE * PtrPinnedReadBuffer = NULL;

	PEXT2_INODE		PtrTempInode;

	//	Buffer Control Block
	PBCB PtrBCB = NULL;

	LARGE_INTEGER VolumeByteOffset, TempOffset;

	ULONG LogicalBlockSize = 0;

	ULONG NumberOfBytesToRead = 0;
	ULONG Difference = 0;

	ULONG GroupNo;
	int Index;

	try
	{
		ASSERT(PtrVcb);
		ASSERT(PtrVcb->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);

		//	Inode numbers start at 1 and not from 0
		//	Hence 1 is subtracted from InodeNo to get a zero based index...
		GroupNo = ( InodeNo - 1 ) / PtrVcb->InodesPerGroup;

		if( GroupNo >= PtrVcb->NoOfGroups )
		{
			DebugTrace(DEBUG_TRACE_MISC,   "&&&&&& Invalid Inode no. Group no %d - too big", GroupNo );
			DebugTrace(DEBUG_TRACE_MISC,   "Only %d groups available on disk", PtrVcb->NoOfGroups );
			RC = STATUS_UNSUCCESSFUL;
			try_return( RC );
		}

		//if( PtrVcb->InodeTableBlock[ GroupNo ] == 0 )
		if( PtrVcb->PtrGroupDescriptors[ GroupNo ].InodeTablesBlock == 0 )
		{
			DebugTrace(DEBUG_TRACE_MISC,   "&&&&&& Inode Table Group Invalid - Group no %d ", GroupNo );
			RC = STATUS_UNSUCCESSFUL;
			try_return( RC );
		}

		//	Inode numbers start at 1 and not from 0
		//	Hence 1 is subtracted from InodeNo to get a zero based index...
		Index = ( InodeNo - 1 ) - ( GroupNo * PtrVcb->InodesPerGroup );

		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVcb->LogBlockSize;
		NumberOfBytesToRead = sizeof(EXT2_INODE);	//	LogicalBlockSize;

		VolumeByteOffset.QuadPart = PtrVcb->PtrGroupDescriptors[ GroupNo ].InodeTablesBlock
				* LogicalBlockSize + Index * sizeof(EXT2_INODE);
		//VolumeByteOffset.QuadPart = PtrVcb->InodeTableBlock[ GroupNo ] * LogicalBlockSize +
		//	Index * sizeof(EXT2_INODE);
		
		TempOffset.QuadPart = Ext2Align64( VolumeByteOffset.QuadPart, LogicalBlockSize );
		if( TempOffset.QuadPart != VolumeByteOffset.QuadPart )
		{
			//	TempOffset.QuadPart -= LogicalBlockSize;
			Difference = (LONG) (VolumeByteOffset.QuadPart - TempOffset.QuadPart + LogicalBlockSize );
			VolumeByteOffset.QuadPart -= Difference;
			NumberOfBytesToRead += Difference;
		}

		NumberOfBytesToRead = Ext2Align( NumberOfBytesToRead, LogicalBlockSize );

		if( NumberOfBytesToRead > LogicalBlockSize )
		{
			//	Multiple blocks being read in...
			//	Can cause overlap
			//	Watch out!!!!
			Ext2BreakPoint();
		}



		if (!CcMapData( PtrVcb->PtrStreamFileObject,
			&VolumeByteOffset,
			NumberOfBytesToRead,
			TRUE,
			&PtrBCB,
			(PVOID*)&PtrPinnedReadBuffer )) 
		{
			RC = STATUS_UNSUCCESSFUL;
			try_return( RC );
		}
		else
		{
			PtrTempInode = (PEXT2_INODE) ( PtrPinnedReadBuffer + Difference );
			RtlCopyMemory( PtrInode, PtrTempInode , sizeof(EXT2_INODE) );
		}

		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrBCB )
		{
			CcUnpinData( PtrBCB );
			PtrBCB = NULL;
		}

	}
	return RC;
}

/*************************************************************************
*
* Function: Ext2InitializeFCBInodeInfo()
*
* Description:
*	The functions will initialize the FCB with its i-node info
*	provided it hasn't been initialized as yet...
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Arguements:
*	Pointer to FCB
*
* Return Value: None
*
*************************************************************************/
void NTAPI Ext2InitializeFCBInodeInfo (
	PtrExt2FCB	PtrFCB )
{
	PtrExt2VCB			PtrVCB = NULL;
	EXT2_INODE			Inode;
	int i;
	ULONG LogicalBlockSize;

	PtrVCB = PtrFCB->PtrVCB;

	LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

	if( !Ext2IsFlagOn( PtrFCB->FCBFlags, EXT2_FCB_BLOCKS_INITIALIZED ) )
	{
		DebugTrace(DEBUG_TRACE_MISC,   "Reading in the i-node no %d", PtrFCB->INodeNo );

		Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode );	
			
		for( i = 0; i < EXT2_N_BLOCKS ; i++ )
		{
			PtrFCB->IBlock[i] = Inode.i_block[ i ];
		}

		PtrFCB->CreationTime.QuadPart	= ( __int64 )Inode.i_ctime * 10000000;
		PtrFCB->CreationTime.QuadPart	+= Ext2GlobalData.TimeDiff.QuadPart;
		PtrFCB->LastAccessTime.QuadPart	= Ext2GlobalData.TimeDiff.QuadPart + ( ( __int64 ) Inode.i_atime * 10000000);
		PtrFCB->LastWriteTime.QuadPart	= Ext2GlobalData.TimeDiff.QuadPart + ( ( __int64 ) Inode.i_mtime * 10000000);


		PtrFCB->LinkCount = Inode.i_links_count;

		//	Getting the file type...
		if( ! Ext2IsModeRegularFile( Inode.i_mode ) )
		{  
			//	Not a reqular file...
			if( Ext2IsModeDirectory( Inode.i_mode) )
			{
				//	Directory...
				Ext2SetFlag( PtrFCB->FCBFlags, EXT2_FCB_DIRECTORY );
			}
			else
			{
				//	Special File...
				//	Treated with respect... ;)
				//
				Ext2SetFlag( PtrFCB->FCBFlags, EXT2_FCB_SPECIAL_FILE );
			}

		}
		if( Ext2IsModeHidden( Inode.i_mode ) )
		{
			Ext2SetFlag( PtrFCB->FCBFlags, EXT2_FCB_HIDDEN_FILE );
		}
		if( Ext2IsModeReadOnly( Inode.i_mode ) )
		{
			Ext2SetFlag( PtrFCB->FCBFlags, EXT2_FCB_READ_ONLY );
		}
		

		PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart = Inode.i_size;
		Ext2SetFlag( PtrFCB->FCBFlags, EXT2_FCB_BLOCKS_INITIALIZED );
		PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart = Inode.i_blocks * 512;

		if( PtrFCB->IBlock[ EXT2_IND_BLOCK ] )
		{
			PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart -= LogicalBlockSize / 512;
		}
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [metadata]", Inode );
	}
}

/*************************************************************************
*
* Function: Ext2AllocInode()
*
* Description:
*	The functions will allocate a new on-disk i-node
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
*
* Arguements:
*	Parent Inode no
*
* Return Value: The new i-node no or zero
*
*************************************************************************/
ULONG NTAPI Ext2AllocInode( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				ParentINodeNo )
{
	ULONG InodeNo = 0;

	//	Buffer Control Block
	PBCB		PtrBitmapBCB = NULL;
	BYTE *		PtrBitmapBuffer = NULL;

	LARGE_INTEGER VolumeByteOffset;
	ULONG LogicalBlockSize = 0;
	ULONG NumberOfBytesToRead = 0;
	
	if( PtrVCB->FreeInodesCount == 0)
	{
		//
		//	No Free Inodes left...
		//	Fail request...
		//
		return 0;
	}

	try
	{
		//	unsigned int DescIndex ;
		BOOLEAN Found = FALSE;
		ULONG Block;
		ULONG GroupNo;

		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
		
		for( GroupNo = 0; PtrVCB->NoOfGroups; GroupNo++ )
		{
			if( PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeInodesCount )
				break;
		}

		VolumeByteOffset.QuadPart = 
			PtrVCB->PtrGroupDescriptors[ GroupNo ].InodeBitmapBlock * LogicalBlockSize;
		
		NumberOfBytesToRead = PtrVCB->InodesCount / PtrVCB->NoOfGroups;

		if( NumberOfBytesToRead % 8 )
		{
			NumberOfBytesToRead = ( NumberOfBytesToRead / 8 ) + 1;
		}
		else
		{
			NumberOfBytesToRead = ( NumberOfBytesToRead / 8 ) ;
		}

		for( Block = 0; !Found && Block < Ext2Align( NumberOfBytesToRead , LogicalBlockSize ); 
				Block += LogicalBlockSize, VolumeByteOffset.QuadPart += LogicalBlockSize)
		{	
			//
			//	Read in the bitmap block...
			//
			ULONG i, j;
			BYTE Bitmap;
			
			if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   LogicalBlockSize, //NumberOfBytesToRead,
				   TRUE,
				   &PtrBitmapBCB,
				   (PVOID*)&PtrBitmapBuffer ) )
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
				return 0;
			}
			
			//
			//	Is there a free inode...
			//	
			for( i = 0; !Found && i < LogicalBlockSize && 
						i + (Block * LogicalBlockSize) < NumberOfBytesToRead; i++ )
			{
				Bitmap = PtrBitmapBuffer[i];
				if( Bitmap != 0xff )
				{
					//
					//	Found a free inode...
					for( j = 0; !Found && j < 8; j++ )
					{
						if( ( Bitmap & 0x01 ) == 0 )
						{
							//
							//	Found...
							Found = TRUE;

							//	Inode numbers start at 1 and not from 0
							//	Hence 1 is addded to j 
							InodeNo = ( ( ( Block * LogicalBlockSize) + i ) * 8) + j + 1 +
								( GroupNo * PtrVCB->InodesPerGroup );
						
							//	Update the inode on the disk...
							Bitmap = 1 << j;
							PtrBitmapBuffer[i] |= Bitmap;
							
							CcSetDirtyPinnedData( PtrBitmapBCB, NULL );
							Ext2SaveBCB( PtrIrpContext, PtrBitmapBCB, PtrVCB->PtrStreamFileObject );

							//
							//	Should update the bitmaps in the other groups too...
							//
							break;
						}
						Bitmap = Bitmap >> 1;
					}
				}
			}
			//
			//	Unpin the BCB...
			//
			if( PtrBitmapBCB )
			{
				CcUnpinData( PtrBitmapBCB );
				PtrBitmapBCB = NULL;
			}
		}

		{
			//
			//	Updating the Inode count in the Group Descriptor...
			//	
			PBCB					PtrDescriptorBCB = NULL;
			PEXT2_GROUP_DESCRIPTOR	PtrGroupDescriptor = NULL;

			PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeInodesCount--;

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

			if (!CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrDescriptorBCB ,
				   (PVOID*)&PtrGroupDescriptor )) 
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
				//
				//	Ignore this error...
				//	Not fatal...
			}
			else
			{
				PtrGroupDescriptor[ GroupNo ].bg_free_inodes_count = 
					PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeInodesCount; 
				//
				//	Not synchronously flushing this information...
				//	Lazy writing will do...
				//
				CcSetDirtyPinnedData( PtrDescriptorBCB, NULL );
				CcUnpinData( PtrDescriptorBCB );
				PtrDescriptorBCB = NULL;
			}
		}


		//
		//	Update the Inode count...
		//	in the Super Block...
		//
		{
			//	Ext2 Super Block information...
			PEXT2_SUPER_BLOCK	PtrSuperBlock = NULL;
			PBCB				PtrSuperBlockBCB = NULL;

			PtrVCB->FreeInodesCount--;
			//	Reading in the super block...
			VolumeByteOffset.QuadPart = 1024;

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
			}
			else
			{
				PtrSuperBlock->s_free_inodes_count = PtrVCB->FreeInodesCount;
				CcSetDirtyPinnedData( PtrSuperBlockBCB, NULL );
				Ext2SaveBCB( PtrIrpContext, PtrSuperBlockBCB, PtrVCB->PtrStreamFileObject );
				if( PtrSuperBlockBCB )
				{
					CcUnpinData( PtrSuperBlockBCB );
					PtrSuperBlockBCB = NULL;
				}
				
			}
		}
	}
	finally
	{
		if( PtrBitmapBCB )
		{
			CcUnpinData( PtrBitmapBCB );
			PtrBitmapBCB = NULL;
		}
	}
	DebugTrace( DEBUG_TRACE_SPECIAL, " Allocating an inode - I-Node no : %ld", InodeNo );
	
	return InodeNo;

}

/*************************************************************************
*
* Function: Ext2DeallocInode()
*
* Description:
*	The functions will deallocate an i-node 
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Return Value: Success / Failure...
*
*************************************************************************/
BOOLEAN NTAPI Ext2DeallocInode( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				INodeNo )
{
	BOOLEAN		RC = TRUE;
	
	//	Buffer Control Block
	PBCB		PtrBitmapBCB = NULL;
	BYTE *		PtrBitmapBuffer = NULL;	

	LARGE_INTEGER VolumeByteOffset;
	ULONG		LogicalBlockSize = 0;
	
	DebugTrace( DEBUG_TRACE_SPECIAL, " Deallocating an inode - I-Node no : %ld", INodeNo );

	try
	{
		ULONG	BlockIndex ;
		ULONG	BitmapIndex;
		ULONG	GroupNo;
		BYTE	Bitmap;
		
		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

		GroupNo = INodeNo / PtrVCB->InodesPerGroup;
		INodeNo = INodeNo % PtrVCB->InodesPerGroup;

		BitmapIndex =  (INodeNo-1) / 8;
		Bitmap = 1 << ( (INodeNo-1) % 8 );
		BlockIndex = BitmapIndex / LogicalBlockSize;
		//	Adjusting to index into the Logical block that contains the bitmap
		BitmapIndex = BitmapIndex - ( BlockIndex * LogicalBlockSize );

		VolumeByteOffset.QuadPart = 
			( PtrVCB->PtrGroupDescriptors[ GroupNo ].InodeBitmapBlock + BlockIndex ) 
			* LogicalBlockSize;

		//
		//	Read in the bitmap block...
		//
		if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				&VolumeByteOffset,
				LogicalBlockSize,	//	Just the block that contains the bitmap will do...
				TRUE,				//	Can Wait...
				&PtrBitmapBCB,
				(PVOID*)&PtrBitmapBuffer ) )
		{
			//	Unable to Pin the data into the cache...
			try_return (RC = FALSE);
		}

		//
		//	Locate the inode...
		//	This inode is in the byte PtrBitmapBuffer[ BitmapIndex ]
		if( ( PtrBitmapBuffer[ BitmapIndex ] & Bitmap ) == 0)
		{
			//	This shouldn't have been so...
			//	The inode was never allocated!
			//	How to deallocate something that hasn't been allocated? 
			//	Hmmm...	;)
			//	Ignore this error...
			try_return (RC = TRUE);
		}


		//	Setting the bit for the inode...
		PtrBitmapBuffer[ BitmapIndex ] &= (~Bitmap);

		//	Update the cache...
		CcSetDirtyPinnedData( PtrBitmapBCB, NULL );

		//	Save up the BCB for forcing a synchronous write...
		//	Before completing the IRP...
		Ext2SaveBCB( PtrIrpContext, PtrBitmapBCB, PtrVCB->PtrStreamFileObject );


		if( PtrBitmapBCB )
		{
			CcUnpinData( PtrBitmapBCB );
			PtrBitmapBCB = NULL;
		}
		
		{
			//
			//	Updating the Inode count in the Group Descriptor...
			//	
			PBCB					PtrDescriptorBCB = NULL;
			PEXT2_GROUP_DESCRIPTOR	PtrGroupDescriptor = NULL;
			ULONG					NumberOfBytesToRead = 0;

			PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeInodesCount++;

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

			if (!CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrDescriptorBCB ,
				   (PVOID*)&PtrGroupDescriptor )) 
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
				//
				//	Ignore this error...
				//	Not fatal...
			}
			else
			{
				PtrGroupDescriptor[ GroupNo ].bg_free_inodes_count = 
					PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeInodesCount; 
				//
				//	Not synchronously flushing this information...
				//	Lazy writing will do...
				//
				CcSetDirtyPinnedData( PtrDescriptorBCB, NULL );
				CcUnpinData( PtrDescriptorBCB );
				PtrDescriptorBCB = NULL;
			}
		}
		

		//
		//	Update the Inode count...
		//	in the Super Block
		//	and in the VCB
		//
		{
			//	Ext2 Super Block information...
			PEXT2_SUPER_BLOCK	PtrSuperBlock = NULL;
			PBCB				PtrSuperBlockBCB = NULL;
			ULONG				NumberOfBytesToRead = 0;

			PtrVCB->FreeInodesCount++;

			//	Reading in the super block...
			VolumeByteOffset.QuadPart = 1024;
			NumberOfBytesToRead = Ext2Align( sizeof( EXT2_SUPER_BLOCK ), LogicalBlockSize );

			if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrSuperBlockBCB,
				   (PVOID*)&PtrSuperBlock ) )
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
			}
			else
			{
				PtrSuperBlock->s_free_inodes_count = PtrVCB->FreeInodesCount;
				CcSetDirtyPinnedData( PtrSuperBlockBCB, NULL );
				Ext2SaveBCB( PtrIrpContext, PtrSuperBlockBCB, PtrVCB->PtrStreamFileObject );
				if( PtrSuperBlockBCB )
				{
					CcUnpinData( PtrSuperBlockBCB );
					PtrSuperBlockBCB = NULL;
				}
				
			}
		}
		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrBitmapBCB )
		{
			CcUnpinData( PtrBitmapBCB );
			PtrBitmapBCB = NULL;
		}
	}
	return RC;
}

/*************************************************************************
*
* Function: Ext2WriteInode()
*
* Description:
*	The functions will write an i-node to disk
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
*
* Return Value: Success / Failure...
*
*************************************************************************/
NTSTATUS NTAPI Ext2WriteInode(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVcb,			//	the Volume Control Block
	uint32				InodeNo,		//	The Inode no
	PEXT2_INODE			PtrInode		//	The Inode Buffer
	)					
{
	//	The Status to be returned...
	NTSTATUS RC = STATUS_SUCCESS;

	//	The Read Buffer Pointer
	BYTE * PtrPinnedBuffer = NULL;

	//	Buffer Control Block
	PBCB PtrBCB = NULL;

	LARGE_INTEGER VolumeByteOffset, TempOffset;

	ULONG LogicalBlockSize = 0;
	ULONG NumberOfBytesToRead = 0;
	ULONG Difference = 0;
	ULONG GroupNo;
	int Index;

	try
	{
		DebugTrace( DEBUG_TRACE_SPECIAL, "Writing and updating an inode - I-Node no : %ld", InodeNo );

		ASSERT(PtrVcb);
		ASSERT(PtrVcb->NodeIdentifier.NodeType == EXT2_NODE_TYPE_VCB);
		GroupNo = InodeNo / PtrVcb->InodesPerGroup;

		if( GroupNo >= PtrVcb->NoOfGroups )
		{
			DebugTrace(DEBUG_TRACE_MISC,   "&&&&&& Invalid Inode no. Group no %d - too big", GroupNo );
			DebugTrace(DEBUG_TRACE_MISC,   "Only %d groups available on disk", PtrVcb->NoOfGroups );
			RC = STATUS_UNSUCCESSFUL;
			try_return( RC );
		}

		if( PtrVcb->PtrGroupDescriptors[ GroupNo ].InodeTablesBlock == 0 )
		{
			DebugTrace(DEBUG_TRACE_MISC,   "&&&&&& Inode Table Group Invalid - Group no %d ", GroupNo );
			RC = STATUS_UNSUCCESSFUL;
			try_return( RC );
		}

		Index = ( InodeNo - 1 ) - ( GroupNo * PtrVcb->InodesPerGroup );

		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVcb->LogBlockSize;
		NumberOfBytesToRead = sizeof(EXT2_INODE);

		VolumeByteOffset.QuadPart = PtrVcb->PtrGroupDescriptors[ GroupNo ].InodeTablesBlock
				* LogicalBlockSize + Index * sizeof(EXT2_INODE);
		
		TempOffset.QuadPart = Ext2Align64( VolumeByteOffset.QuadPart, LogicalBlockSize );
		if( TempOffset.QuadPart != VolumeByteOffset.QuadPart )
		{
			//	TempOffset.QuadPart -= LogicalBlockSize;
			Difference = (LONG) (VolumeByteOffset.QuadPart - TempOffset.QuadPart + LogicalBlockSize );
			VolumeByteOffset.QuadPart -= Difference;
			NumberOfBytesToRead += Difference;
		}

		NumberOfBytesToRead = Ext2Align( NumberOfBytesToRead, LogicalBlockSize );

		if( NumberOfBytesToRead > LogicalBlockSize )
		{
			//	Multiple blocks being read in...
			//	Can cause overlap
			//	Watch out!!!!
			Ext2BreakPoint();
		}

		if( !CcPinRead( PtrVcb->PtrStreamFileObject,
					&VolumeByteOffset,
					NumberOfBytesToRead,
					TRUE,			//	Can Wait...
					&PtrBCB,
					(PVOID*)&PtrPinnedBuffer ) )
		{
			RC = STATUS_UNSUCCESSFUL;
			try_return( RC );
		}
		else
		{
			RtlCopyMemory( PtrPinnedBuffer + Difference, PtrInode, sizeof(EXT2_INODE) );
			CcSetDirtyPinnedData( PtrBCB, NULL );
			Ext2SaveBCB( PtrIrpContext, PtrBCB, PtrVcb->PtrStreamFileObject );
		}
	
		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrBCB )
		{
			CcUnpinData( PtrBCB );
			PtrBCB = NULL;
		}

	}
	return RC;
}


/*************************************************************************
*
* Function: Ext2MakeNewDirectoryEntry()
*
* Description:
*	The functions will make a new directory entry in a directory file...
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
*
* Return Value: Success / Failure...
*
*************************************************************************/
BOOLEAN NTAPI Ext2MakeNewDirectoryEntry(
	PtrExt2IrpContext		PtrIrpContext,	//	The Irp context
	PtrExt2FCB				PtrParentFCB,	//	Parent Folder FCB
	PFILE_OBJECT			PtrFileObject,	//	Parent Folder Object
	PUNICODE_STRING			PtrName,		//	New entry's name
	ULONG					Type,			//	The type of the new entry
	ULONG					NewInodeNo)		//	The inode no of the new entry...
{
	PBCB				PtrLastBlockBCB	= NULL;
	BYTE *	 			PtrLastBlock = NULL;
	EXT2_DIR_ENTRY		DirEntry;
	PEXT2_DIR_ENTRY		PtrTempDirEntry;
	
	ULONG				BlockNo = 0;
	ULONG				i;
	PtrExt2VCB			PtrVCB;
	LARGE_INTEGER		VolumeByteOffset;
	unsigned long		LogicalBlockSize = 0;
	BOOLEAN				RC = FALSE;
	
	USHORT	HeaderLength = sizeof( EXT2_DIR_ENTRY );
	USHORT	NewEntryLength = 0; 
	USHORT	MinLength	= 0;
	#define ActualLength (PtrTempDirEntry->rec_len)
	#define NameLength  (PtrTempDirEntry->name_len)

	try
	{
		ASSERT( PtrFileObject );

		DebugTrace( DEBUG_TRACE_SPECIAL, "Making directory entry: %S", PtrName->Buffer );

		PtrVCB = PtrParentFCB->PtrVCB;
		AssertVCB( PtrVCB);

		HeaderLength = sizeof( EXT2_DIR_ENTRY ) -
						  (sizeof( char ) * EXT2_NAME_LEN);
		//	1. Setting up the entry...
		NewEntryLength = sizeof( EXT2_DIR_ENTRY ) - ( sizeof( char ) * ( EXT2_NAME_LEN - (PtrName->Length / 2) ) );
		//	Length should be a multiplicant of 4
		NewEntryLength = ((NewEntryLength + 3 ) & 0xfffffffc);

		RtlZeroMemory( &DirEntry, sizeof( EXT2_DIR_ENTRY ) );

		DirEntry.file_type = (BYTE) Type;
		DirEntry.inode = NewInodeNo;
		DirEntry.name_len = (BYTE)(PtrName->Length / 2 );	//	Does not include a NULL
		
		//	DirEntry.rec_len = (USHORT) NewEntryLength;

		for( i = 0; ; i++ )
		{
			if( i < (ULONG)( PtrName->Length / 2 ) )
			{
				DirEntry.name[i] = (CHAR) PtrName->Buffer[i];
			}
			else
			{
				//DirEntry.name[i] = 0;	//	Entry need not be zero terminated...
				break;
			}
		}

		//
		//	2. Read the block in the directory...
		//	Initiate Caching...
		if ( PtrFileObject->PrivateCacheMap == NULL )
		{
			CcInitializeCacheMap(
				PtrFileObject, 
				(PCC_FILE_SIZES)(&(PtrParentFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize)),
				TRUE,									// We utilize pin access for directories
				&(Ext2GlobalData.CacheMgrCallBacks),	// callbacks
				PtrParentFCB );							// The context used in callbacks
		}

		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
		if( PtrParentFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart > 0 )
		{
			BlockNo = (ULONG) ( (PtrParentFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) / LogicalBlockSize) ;
		}
		else
		{
			//	This directory doesn't have any data blocks...
			//	Allocate a new block...
			if( !Ext2AddBlockToFile( PtrIrpContext, PtrVCB, PtrParentFCB, PtrFileObject, TRUE ) )
			{
				try_return( RC = FALSE );
			}
			else
			{
				//	Bring in the newly allocated block to the cache...
				VolumeByteOffset.QuadPart = 0;

				if( !CcPreparePinWrite( 
					PtrFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,			//	Zero out the block...
					TRUE,			//	Can Wait...
					&PtrLastBlockBCB,
					(PVOID*)&PtrLastBlock ) )
				{
					try_return( RC = FALSE );
				}

				DirEntry.rec_len = (USHORT)LogicalBlockSize;
				RtlCopyBytes( PtrLastBlock, &DirEntry, NewEntryLength);
				CcSetDirtyPinnedData( PtrLastBlockBCB, NULL );
				Ext2SaveBCB( PtrIrpContext, PtrLastBlockBCB, PtrFileObject );
				try_return( RC = TRUE );
			}
		}

		VolumeByteOffset.QuadPart = BlockNo * LogicalBlockSize;
		CcMapData(	PtrFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,
					&PtrLastBlockBCB,
					(PVOID*)&PtrLastBlock );

		for( i = 0 ; i < LogicalBlockSize; )
		{
			PtrTempDirEntry = (PEXT2_DIR_ENTRY) &PtrLastBlock[ i ];

			MinLength = HeaderLength + NameLength;
			MinLength = ( HeaderLength + NameLength + 3 ) & 0xfffffffc;

			
			if( PtrTempDirEntry->rec_len == 0 )
			{
				if( i == 0 )
				{
					//	Must be an empty Block...
					//	Insert here...
					//	---------------->>>
					
					CcPinMappedData( PtrFileObject,
								   &VolumeByteOffset,
								   LogicalBlockSize,
								   TRUE,
								   &PtrLastBlockBCB );

					DirEntry.rec_len = (USHORT)LogicalBlockSize;
					
					RtlCopyBytes( PtrLastBlock, &DirEntry, NewEntryLength);
					CcSetDirtyPinnedData( PtrLastBlockBCB, NULL );
					Ext2SaveBCB( PtrIrpContext, PtrLastBlockBCB, PtrFileObject );
					try_return( RC = TRUE );
				}
				else
				{
					//	This shouldn't be so...
					//	Something is wrong...
					//	Fail this request...
					try_return( RC = FALSE );
				}
			}
			if( ActualLength - MinLength >= NewEntryLength )
			{
				//	Insert here...
				//	---------------->

				//	Getting ready for updation...
				CcPinMappedData( PtrFileObject,
							   &VolumeByteOffset,
							   LogicalBlockSize,
							   TRUE,
							   &PtrLastBlockBCB );


				DirEntry.rec_len = ActualLength - MinLength;

				//	Updating the current last entry
				PtrTempDirEntry->rec_len = MinLength;
				i += PtrTempDirEntry->rec_len;
				
				//	Making the new entry...
				RtlCopyBytes( (PtrLastBlock + i) , &DirEntry, NewEntryLength);
				CcSetDirtyPinnedData( PtrLastBlockBCB, NULL );
				Ext2SaveBCB( PtrIrpContext, PtrLastBlockBCB, PtrFileObject );
				try_return( RC = TRUE );

			}
			i += PtrTempDirEntry->rec_len;
		}

		//	Will have to allocate a new block...
		//	Old block does not have enough space..
		if( !Ext2AddBlockToFile( PtrIrpContext, PtrVCB, PtrParentFCB, PtrFileObject, TRUE ) )
		{
			try_return( RC = FALSE );
		}
		else
		{
			//	unpin the previously pinned block
			CcUnpinData( PtrLastBlockBCB );
			PtrLastBlockBCB = NULL;

			//	Bring in the newly allocated block to the cache...
			VolumeByteOffset.QuadPart += LogicalBlockSize;
			if( !CcPreparePinWrite( 
				PtrFileObject,
				&VolumeByteOffset,
				LogicalBlockSize,
				TRUE,			//	Zero out the block...
				TRUE,			//	Can Wait...
				&PtrLastBlockBCB,
				(PVOID*)&PtrLastBlock ) )
			{
				try_return( RC = FALSE );
			}

			DirEntry.rec_len = (USHORT)LogicalBlockSize;
			RtlCopyBytes( PtrLastBlock, &DirEntry, NewEntryLength);
			CcSetDirtyPinnedData( PtrLastBlockBCB, NULL );
			Ext2SaveBCB( PtrIrpContext, PtrLastBlockBCB, PtrFileObject );
			try_return( RC = TRUE );
		}
		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrLastBlockBCB )
		{
			CcUnpinData( PtrLastBlockBCB );
			PtrLastBlockBCB = NULL;
		}
	}
	if( RC == FALSE )
	{
		DebugTrace( DEBUG_TRACE_ERROR, "Failed to making directory entry: %S", PtrName->Buffer );
	}
	return RC;
}


BOOLEAN NTAPI Ext2FreeDirectoryEntry(
	PtrExt2IrpContext		PtrIrpContext,
	PtrExt2FCB				PtrParentFCB,
	PUNICODE_STRING			PtrName)
{

	PBCB				PtrDataBlockBCB	= NULL;
	BYTE *	 			PtrDataBlock = NULL;
	PFILE_OBJECT		PtrFileObject = NULL;
	LONGLONG			ByteOffset = 0;
	PtrExt2VCB			PtrVCB;
	LARGE_INTEGER		VolumeByteOffset;
	unsigned long		LogicalBlockSize = 0;
	BOOLEAN				RC = FALSE;
	

	try
	{
		DebugTrace( DEBUG_TRACE_SPECIAL, "Freeing directory entry: %S", PtrName->Buffer );

		PtrVCB = PtrParentFCB->PtrVCB;
		AssertVCB( PtrVCB);
		
		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

		PtrFileObject = PtrParentFCB->DcbFcb.Dcb.PtrDirFileObject;
		if( PtrFileObject == NULL )
		{
			return FALSE;
		}
		

		//
		//	1. Read the block in the directory...
		//	Initiate Caching...
		if ( PtrFileObject->PrivateCacheMap == NULL )
		{
			CcInitializeCacheMap(
				PtrFileObject, 
				(PCC_FILE_SIZES)(&(PtrParentFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize)),
				TRUE,									// We utilize pin access for directories
				&(Ext2GlobalData.CacheMgrCallBacks),	// callbacks
				PtrParentFCB );							// The context used in callbacks
		}

		for( ByteOffset = 0; 
			 ByteOffset < PtrParentFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart;
			 ByteOffset += LogicalBlockSize )
		{
			ULONG Index = 0;
			PEXT2_DIR_ENTRY PtrDirEntry = NULL;


			VolumeByteOffset.QuadPart = ByteOffset;
			
			CcPinRead(	PtrFileObject,
						&VolumeByteOffset,
						LogicalBlockSize,
						TRUE,
						&PtrDataBlockBCB,
						(PVOID*)&PtrDataBlock );
			while( Index < LogicalBlockSize )
			{
				ULONG i;
				//	Parse...
				PtrDirEntry = (PEXT2_DIR_ENTRY) &PtrDataBlock[ Index ];
				Index += PtrDirEntry->rec_len;

				if( PtrDirEntry->inode == 0 )
				{
					//	This is a deleted entry...
					continue;
				}
				if( ( PtrName->Length/2 ) != PtrDirEntry->name_len )
					continue;
				for( i = 0; ; i++ )
				{
					if( PtrDirEntry->name_len == i )
					{
						//	Remove the entry by setting the inode no to zero
						PtrDirEntry->inode = 0;

						//	Update the disk
						CcSetDirtyPinnedData( PtrDataBlockBCB , NULL );
						Ext2SaveBCB( PtrIrpContext, PtrDataBlockBCB, PtrFileObject );
						CcUnpinData( PtrDataBlockBCB );
						PtrDataBlockBCB = NULL;

						//	Return to caller...
						try_return( RC = TRUE );
					}
					if( PtrName->Buffer[i] != PtrDirEntry->name[i] )
					{
						break;
					}
				}
			}
			CcUnpinData( PtrDataBlockBCB );
			PtrDataBlockBCB = NULL;
		}
		try_return( RC = FALSE );

		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrDataBlockBCB )
		{
			CcUnpinData( PtrDataBlockBCB );
			PtrDataBlockBCB = NULL;
		}
	}
	return RC;
}

/*************************************************************************
*
* Function: Ext2AddBlockToFile()
*
* Description:
*	The functions will add a block to a file...
*	It will update the allocation size but not the file size...
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
*
* Return Value: Success / Failure...
*
*************************************************************************/
BOOLEAN NTAPI Ext2AddBlockToFile(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	PtrExt2FCB			PtrFCB,
	PFILE_OBJECT		PtrFileObject,
	BOOLEAN				UpdateFileSize)
{
	BOOLEAN			RC = TRUE; 

	ULONG			NewBlockNo = 0;
	LARGE_INTEGER	VolumeByteOffset;
	ULONG			LogicalBlockSize = 0;
	ULONG			NoOfBlocks = 0;
	EXT2_INODE		Inode;

	ULONG	DirectBlocks = 0;
	ULONG	SingleIndirectBlocks = 0;
	ULONG	DoubleIndirectBlocks = 0;
	ULONG	TripleIndirectBlocks = 0;
	
	ULONG	*PtrSIBBuffer = NULL;
	PBCB	PtrSIBBCB = NULL;
	ULONG	*PtrDIBBuffer = NULL;
	PBCB	PtrDIBBCB = NULL;


	LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
	DirectBlocks =  EXT2_NDIR_BLOCKS ;
	SingleIndirectBlocks = LogicalBlockSize / sizeof( ULONG );
	DoubleIndirectBlocks = SingleIndirectBlocks * LogicalBlockSize / sizeof( ULONG );
	TripleIndirectBlocks = DoubleIndirectBlocks * LogicalBlockSize / sizeof( ULONG );

	try
	{
		if( PtrFCB && PtrFCB->FCBName->ObjectName.Length )
		{
			DebugTrace( DEBUG_TRACE_SPECIAL, "Adding Blocks to file  %S", PtrFCB->FCBName->ObjectName.Buffer );
		}

		Ext2InitializeFCBInodeInfo( PtrFCB );

		//	Allocate a block...
		NewBlockNo = Ext2AllocBlock( PtrIrpContext, PtrVCB, 1 );

		if( NewBlockNo == 0 )
		{
			try_return (RC = FALSE );
		}

		//	No of blocks CURRENTLY allocated...
		NoOfBlocks = (ULONG) PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart / LogicalBlockSize;
		
		
		if( NoOfBlocks < EXT2_NDIR_BLOCKS )
		{
			//
			//	A direct data block will do...
			//
			
			PtrFCB->IBlock[ NoOfBlocks ] = NewBlockNo;
			
			//	Update the inode...
			Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode	);
			Inode.i_block[ NoOfBlocks ] = NewBlockNo;
			Inode.i_blocks += ( LogicalBlockSize / 512 );
			PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart += LogicalBlockSize;
			if( UpdateFileSize )
			{
				Inode.i_size += LogicalBlockSize;
				PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart += LogicalBlockSize;
			}

			
			if( PtrFileObject->PrivateCacheMap != NULL)
			{
				//
				//	Caching has been initiated...
				//	Let the Cache manager in on these changes...
				//	
				CcSetFileSizes( PtrFileObject, (PCC_FILE_SIZES)&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize));
			}
			
			
			//	Updating the inode...
			if( NT_SUCCESS( Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode	) ) )
			{
				try_return (RC = TRUE);
			}
			else
			{
				try_return (RC = FALSE );
			}
			
		}
		else if( NoOfBlocks < (DirectBlocks + SingleIndirectBlocks) )
		{
			//
			//	A single indirect data block will do...
			Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode	);

			if( PtrFCB->IBlock[ EXT2_IND_BLOCK ] == 0 )
			{
				//	A Single Indirect block should be allocated as well!!
				PtrFCB->IBlock[ EXT2_IND_BLOCK ] = Ext2AllocBlock( PtrIrpContext, PtrVCB, 1 );
				if( PtrFCB->IBlock[ EXT2_IND_BLOCK ] == 0 )
				{
					try_return (RC = FALSE );
				}
				Inode.i_blocks += ( LogicalBlockSize / 512 );

				//	Bring in the new block to the cache
				//	Zero it out
				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_IND_BLOCK ] * LogicalBlockSize;

				if( !CcPreparePinWrite( 
					PtrVCB->PtrStreamFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,			//	Zero out the block...
					TRUE,			//	Can Wait...
					&PtrSIBBCB,
					(PVOID*)&PtrSIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			else
			{
				//	 Just bring in the SIB to the cache
				
				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_IND_BLOCK ] * LogicalBlockSize;

				if( !CcPinRead( PtrVCB->PtrStreamFileObject,
							&VolumeByteOffset,
							LogicalBlockSize,
							TRUE,			//	Can Wait...
							&PtrSIBBCB,
							(PVOID*)&PtrSIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			
			//	Update the inode...
			
			Inode.i_block[ EXT2_IND_BLOCK ] = PtrFCB->IBlock[ EXT2_IND_BLOCK ];
			Inode.i_blocks += ( LogicalBlockSize / 512 );
			PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart += LogicalBlockSize;
			if( UpdateFileSize )
			{
				Inode.i_size += LogicalBlockSize;
				PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart += LogicalBlockSize;
			}
			if( PtrFileObject->PrivateCacheMap != NULL)
			{
				//
				//	Caching has been initiated...
				//	Let the Cache manager in on these changes...
				//	
				CcSetFileSizes( PtrFileObject, (PCC_FILE_SIZES)&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize));
			}

			if( !NT_SUCCESS( Ext2WriteInode( 
				PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode	) ) )
			{
				try_return (RC = FALSE );
			}

			
			//	Update the SIB...
			PtrSIBBuffer[ NoOfBlocks - DirectBlocks ] = NewBlockNo;
			CcSetDirtyPinnedData( PtrSIBBCB, NULL );
			Ext2SaveBCB( PtrIrpContext, PtrSIBBCB, PtrVCB->PtrStreamFileObject );

			try_return (RC = TRUE);

		}
		else if( NoOfBlocks < (DirectBlocks + SingleIndirectBlocks + DoubleIndirectBlocks ) )
		{
			//
			//	A double indirect block will do...
			//
			ULONG SBlockNo;
			ULONG BlockNo;

			Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode	);

			if( PtrFCB->IBlock[ EXT2_DIND_BLOCK ] == 0 )
			{
				//	A double indirect pointer block should be allocated as well!!
				PtrFCB->IBlock[ EXT2_DIND_BLOCK ] = Ext2AllocBlock( PtrIrpContext, PtrVCB, 1 );
				if( PtrFCB->IBlock[ EXT2_DIND_BLOCK ] == 0 )
				{
					try_return (RC = FALSE );
				}
				Inode.i_blocks += ( LogicalBlockSize / 512 );

				//	Bring in the new block to the cache
				//	Zero it out
				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;

				if( !CcPreparePinWrite( 
					PtrVCB->PtrStreamFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,			//	Zero out the block...
					TRUE,			//	Can Wait...
					&PtrDIBBCB,
					(PVOID*)&PtrDIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			else
			{
				//	 Just bring in the DIB to the cache
				
				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;

				if( !CcPinRead( PtrVCB->PtrStreamFileObject,
							&VolumeByteOffset,
							LogicalBlockSize,
							TRUE,			//	Can Wait...
							&PtrDIBBCB,
							(PVOID*)&PtrDIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			
			//	See if a single indirect 'pointer' block 
			//	should also be allocated...
			BlockNo = ( NoOfBlocks - DirectBlocks - SingleIndirectBlocks );
			SBlockNo = BlockNo / SingleIndirectBlocks;
			if( BlockNo % SingleIndirectBlocks )
			{
				//	A single indirect 'pointer' block 
				//	should also be allocated...
				PtrDIBBuffer[SBlockNo] = Ext2AllocBlock( PtrIrpContext, PtrVCB, 1 );
				CcSetDirtyPinnedData( PtrDIBBCB, NULL );
				VolumeByteOffset.QuadPart = PtrDIBBuffer[SBlockNo] * LogicalBlockSize;

				Inode.i_blocks += ( LogicalBlockSize / 512 );

				if( !CcPreparePinWrite( 
					PtrVCB->PtrStreamFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,					//	Zero out the block...
					TRUE,					//	Can Wait...
					&PtrSIBBCB,
					(PVOID*)&PtrSIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			else
			{
				VolumeByteOffset.QuadPart = PtrDIBBuffer[SBlockNo] * LogicalBlockSize;
				if( !CcPinRead( PtrVCB->PtrStreamFileObject,
							&VolumeByteOffset,
							LogicalBlockSize,
							TRUE,				//	Can Wait...
							&PtrSIBBCB,
							(PVOID*)&PtrSIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			BlockNo = BlockNo % SingleIndirectBlocks;
			
			//	Update the inode...
			
			Inode.i_block[ EXT2_DIND_BLOCK ] = PtrFCB->IBlock[ EXT2_DIND_BLOCK ];
			Inode.i_blocks += ( LogicalBlockSize / 512 );
			PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart += LogicalBlockSize;
			if( UpdateFileSize )
			{
				Inode.i_size += LogicalBlockSize;
				PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart += LogicalBlockSize;
			}
			if( PtrFileObject->PrivateCacheMap != NULL)
			{
				//
				//	Caching has been initiated...
				//	Let the Cache manager in on these changes...
				//	
				CcSetFileSizes( PtrFileObject, (PCC_FILE_SIZES)&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize));
			}

			if( !NT_SUCCESS( Ext2WriteInode( 
				PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode	) ) )
			{
				try_return (RC = FALSE );
			}

			
			//	Update the SIB...
			PtrSIBBuffer[ BlockNo ] = NewBlockNo;
			CcSetDirtyPinnedData( PtrSIBBCB, NULL );
			Ext2SaveBCB( PtrIrpContext, PtrSIBBCB, PtrVCB->PtrStreamFileObject );
			Ext2SaveBCB( PtrIrpContext, PtrDIBBCB, PtrVCB->PtrStreamFileObject );

			try_return (RC = TRUE);

		}
		else
		{	
			//
			//	A Triple Indirect block is required
			//
			ULONG SBlockNo;
			ULONG BlockNo;

			//	This is not supported as yet...
			try_return (RC = FALSE);

			Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode	);

			if( PtrFCB->IBlock[ EXT2_TIND_BLOCK ] == 0 )
			{
				//	A double indirect pointer block should be allocated as well!!
				PtrFCB->IBlock[ EXT2_DIND_BLOCK ] = Ext2AllocBlock( PtrIrpContext, PtrVCB, 1 );
				if( PtrFCB->IBlock[ EXT2_DIND_BLOCK ] == 0 )
				{
					try_return (RC = FALSE );
				}
				Inode.i_blocks += ( LogicalBlockSize / 512 );

				//	Bring in the new block to the cache
				//	Zero it out
				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;

				if( !CcPreparePinWrite( 
					PtrVCB->PtrStreamFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,			//	Zero out the block...
					TRUE,			//	Can Wait...
					&PtrDIBBCB,
					(PVOID*)&PtrDIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			else
			{
				//	 Just bring in the DIB to the cache
				
				VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;

				if( !CcPinRead( PtrVCB->PtrStreamFileObject,
							&VolumeByteOffset,
							LogicalBlockSize,
							TRUE,			//	Can Wait...
							&PtrDIBBCB,
							(PVOID*)&PtrDIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			
			//	See if a single indirect 'pointer' block 
			//	should also be allocated...
			BlockNo = ( NoOfBlocks - DirectBlocks - SingleIndirectBlocks );
			SBlockNo = BlockNo / SingleIndirectBlocks;
			if( BlockNo % SingleIndirectBlocks )
			{
				//	A single indirect 'pointer' block 
				//	should also be allocated...
				PtrDIBBuffer[SBlockNo] = Ext2AllocBlock( PtrIrpContext, PtrVCB, 1 );
				CcSetDirtyPinnedData( PtrDIBBCB, NULL );
				VolumeByteOffset.QuadPart = PtrDIBBuffer[SBlockNo] * LogicalBlockSize;

				Inode.i_blocks += ( LogicalBlockSize / 512 );

				if( !CcPreparePinWrite( 
					PtrVCB->PtrStreamFileObject,
					&VolumeByteOffset,
					LogicalBlockSize,
					TRUE,			//	Zero out the block...
					TRUE,			//	Can Wait...
					&PtrSIBBCB,
					(PVOID*)&PtrSIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			else
			{
				VolumeByteOffset.QuadPart = PtrDIBBuffer[SBlockNo] * LogicalBlockSize;
				if( !CcPinRead( PtrVCB->PtrStreamFileObject,
							&VolumeByteOffset,
							LogicalBlockSize,
							TRUE,			//	Can Wait...
							&PtrSIBBCB,
							(PVOID*)&PtrSIBBuffer ) )
				{
					try_return( RC = FALSE );
				}
			}
			BlockNo = BlockNo % SingleIndirectBlocks;
			
			//	Update the inode...
			
			Inode.i_block[ EXT2_DIND_BLOCK ] = PtrFCB->IBlock[ EXT2_DIND_BLOCK ];
			Inode.i_blocks += ( LogicalBlockSize / 512 );
			PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize.QuadPart += LogicalBlockSize;
			if( UpdateFileSize )
			{
				Inode.i_size += LogicalBlockSize;
				PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart += LogicalBlockSize;
			}
			if( PtrFileObject->PrivateCacheMap != NULL)
			{
				//
				//	Caching has been initiated...
				//	Let the Cache manager in on these changes...
				//	
				CcSetFileSizes( PtrFileObject, (PCC_FILE_SIZES)&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize));
			}

			if( !NT_SUCCESS( Ext2WriteInode( 
				PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode	) ) )
			{
				try_return (RC = FALSE );
			}

			
			//	Update the SIB...
			PtrSIBBuffer[ BlockNo ] = NewBlockNo;
			CcSetDirtyPinnedData( PtrSIBBCB, NULL );
			Ext2SaveBCB( PtrIrpContext, PtrSIBBCB, PtrVCB->PtrStreamFileObject );
			Ext2SaveBCB( PtrIrpContext, PtrDIBBCB, PtrVCB->PtrStreamFileObject );

			try_return (RC = TRUE);

		}

		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrSIBBCB )
		{
			CcUnpinData( PtrSIBBCB );
			PtrSIBBCB = NULL;
		}
		if( PtrDIBBCB )
		{
			CcUnpinData( PtrDIBBCB );
			PtrDIBBCB = NULL;
		}
	}
	return RC;
}

/*************************************************************************
*
* Function: Ext2AllocBlock()
*
* Description:
*	The functions will allocate a new block 
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
*
* Return Value: Success / Failure...
*
*************************************************************************/
ULONG NTAPI Ext2AllocBlock( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				Count)
{
	//	Buffer Control Block
	PBCB			PtrBitmapBCB = NULL;
	BYTE *			PtrBitmapBuffer = NULL;
	ULONG			BlockNo = 0;
	LARGE_INTEGER	VolumeByteOffset;
	ULONG			LogicalBlockSize = 0;
	ULONG			NumberOfBytesToRead = 0;

	if( PtrVCB->FreeBlocksCount == 0 )
	{
		//
		//	No Free Block left...
		//	Fail request...
		//
		return 0;
	}

	try
	{
		BOOLEAN Found = FALSE;
		ULONG Block;
		ULONG GroupNo;
		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

		for( GroupNo = 0; PtrVCB->NoOfGroups; GroupNo++ )
		{
			if( PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeBlocksCount )
				break;
		}

		VolumeByteOffset.QuadPart = 
			PtrVCB->PtrGroupDescriptors[ GroupNo ].BlockBitmapBlock * LogicalBlockSize;
		
		NumberOfBytesToRead = PtrVCB->BlocksCount / PtrVCB->NoOfGroups;

		if( NumberOfBytesToRead % 8 )
		{
			NumberOfBytesToRead = ( NumberOfBytesToRead / 8 ) + 1;
		}
		else
		{
			NumberOfBytesToRead = ( NumberOfBytesToRead / 8 ) ;
		}
		
		
		for( Block = 0; !Found && Block < Ext2Align( NumberOfBytesToRead , LogicalBlockSize ); 
				Block += LogicalBlockSize, VolumeByteOffset.QuadPart += LogicalBlockSize)			
		{
			//
			//	Read in the block bitmap block...
			ULONG i, j;
			BYTE Bitmap;
					
			if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   LogicalBlockSize,					//	NumberOfBytesToRead,
				   TRUE,
				   &PtrBitmapBCB,
				   (PVOID*)&PtrBitmapBuffer ) )
			{
				DebugTrace(DEBUG_TRACE_ERROR, "Cache read failiure while reading in volume meta data", 0);
				try_return( BlockNo = 0 );
			}

			//
			//	Is there a free block...
			//	
			for( i = 0; !Found && i < LogicalBlockSize && 
					i + (Block * LogicalBlockSize) < NumberOfBytesToRead; i++ )
			{
				Bitmap = PtrBitmapBuffer[i];
				if( Bitmap != 0xff )
				{
					//
					//	Found a free block...
					for( j = 0; !Found && j < 8; j++ )
					{
						if( ( Bitmap & 0x01 ) == 0 )
						{
							//
							//	Found...
							Found = TRUE;
							BlockNo = ( ( ( Block * LogicalBlockSize) + i ) * 8) + j + 1
								+ ( GroupNo * PtrVCB->BlocksPerGroup );

							Bitmap = 1 << j;
							PtrBitmapBuffer[i] |= Bitmap;
							
							CcSetDirtyPinnedData( PtrBitmapBCB, NULL );
							Ext2SaveBCB( PtrIrpContext, PtrBitmapBCB, PtrVCB->PtrStreamFileObject );
							//
							//	Should update the bitmaps in the other groups too...
							//
							break;
						}
						Bitmap = Bitmap >> 1;
					}
				}
			}
			//
			//	Unpin the BCB...
			//
			if( PtrBitmapBCB )
			{
				CcUnpinData( PtrBitmapBCB );
				PtrBitmapBCB = NULL;
			}

		}

		//
		//	Updating the Free Block count in the Group Descriptor...
		//	
		
		{
			PBCB					PtrDescriptorBCB = NULL;
			PEXT2_GROUP_DESCRIPTOR	PtrGroupDescriptor = NULL;
			//
			//	Updating the Free Blocks count in the Group Descriptor...
			//	
			PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeBlocksCount--;

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

			if (!CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrDescriptorBCB ,
				   (PVOID*)&PtrGroupDescriptor )) 
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
				//
				//	Ignore this error...
				//	Not fatal...
			}
			else
			{
				PtrGroupDescriptor[ GroupNo ].bg_free_blocks_count= 
					PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeBlocksCount; 

				//
				//	Not synchronously flushing this information...
				//	Lazy writing will do...
				//
				CcSetDirtyPinnedData( PtrDescriptorBCB, NULL );
				CcUnpinData( PtrDescriptorBCB );
				PtrDescriptorBCB = NULL;
			}
		}
		
		//
		//	Update the Block count
		//	in the super block and in the VCB
		//
		{
			//	Ext2 Super Block information...
			PEXT2_SUPER_BLOCK	PtrSuperBlock = NULL;
			PBCB				PtrSuperBlockBCB = NULL;

			PtrVCB->FreeBlocksCount--;

			//	Reading in the super block...
			VolumeByteOffset.QuadPart = 1024;
			NumberOfBytesToRead = Ext2Align( sizeof( EXT2_SUPER_BLOCK ), LogicalBlockSize );

			if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrSuperBlockBCB,
				   (PVOID*)&PtrSuperBlock ) )
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
			}
			else
			{
				PtrSuperBlock->s_free_blocks_count = PtrVCB->FreeBlocksCount;
				CcSetDirtyPinnedData( PtrSuperBlockBCB, NULL );
				Ext2SaveBCB( PtrIrpContext, PtrSuperBlockBCB, PtrVCB->PtrStreamFileObject );
				if( PtrSuperBlockBCB )
				{
					CcUnpinData( PtrSuperBlockBCB );
					PtrSuperBlockBCB = NULL;
				}
			}
		}

		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrBitmapBCB )
		{
			CcUnpinData( PtrBitmapBCB );
			PtrBitmapBCB = NULL;
		}
		DebugTrace( DEBUG_TRACE_SPECIAL, " Allocating a block - Block no : %ld", BlockNo );
	}
	return BlockNo;
}

/*************************************************************************
*
* Function: Ext2DeallocBlock()
*
* Description:
*	The functions will deallocate a data block 
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Return Value: Success / Failure...
*
*************************************************************************/
BOOLEAN NTAPI Ext2DeallocBlock( 
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2VCB			PtrVCB,
	ULONG				BlockNo )
{
	//	Buffer Control Block
	PBCB			PtrBitmapBCB = NULL;
	BYTE *			PtrBitmapBuffer = NULL;
	BOOLEAN			RC = TRUE;
	LARGE_INTEGER	VolumeByteOffset;
	ULONG			LogicalBlockSize = 0;
	//	ULONG			NumberOfBytesToRead = 0;

	DebugTrace( DEBUG_TRACE_SPECIAL, " Deallocating a block - Block no : %ld", BlockNo );
	
	try
	{
		ULONG	GroupNo;
		ULONG	BlockIndex;
		ULONG	BitmapIndex;
		BYTE	Bitmap;

		LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

		GroupNo = BlockNo / PtrVCB->BlocksPerGroup;
		BlockNo = BlockNo % PtrVCB->BlocksPerGroup;

		Bitmap = 1 << ( (BlockNo-1) % 8 );
		BitmapIndex =  (BlockNo-1) / 8;
		BlockIndex = BitmapIndex / LogicalBlockSize;
		//	Adjusting to index into the Logical block that contains the bitmap
		BitmapIndex = BitmapIndex - ( BlockIndex * LogicalBlockSize );

		VolumeByteOffset.QuadPart = 
			( PtrVCB->PtrGroupDescriptors[ GroupNo ].BlockBitmapBlock + BlockIndex ) 
			* LogicalBlockSize;
		
		//
		//	Read in the bitmap block...
		//
		if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				&VolumeByteOffset,
				LogicalBlockSize,
				TRUE,			//	Can Wait...
				&PtrBitmapBCB,
				(PVOID*)&PtrBitmapBuffer ) )
		{
			//	Unable to Pin the data into the cache...
			try_return (RC = FALSE);
		}

		//
		//	Locate the block 'bit'...
		//	This block 'bit' is in the byte PtrBitmapBuffer[ BitmapIndex ]
		if( ( PtrBitmapBuffer[ BitmapIndex ] & Bitmap ) == 0)
		{
			//	This shouldn't have been so...
			//	The block was never allocated!
			//	How to deallocate something that hasn't been allocated? 
			//	Hmmm...	;)
			//	Ignore this error...
			try_return (RC = TRUE);
		}

		//	Setting the bit for the inode...
		PtrBitmapBuffer[ BitmapIndex ] &= (~Bitmap);

		//	Update the cache...
		CcSetDirtyPinnedData( PtrBitmapBCB, NULL );

		//	Save up the BCB for forcing a synchronous write...
		//	Before completing the IRP...
		Ext2SaveBCB( PtrIrpContext, PtrBitmapBCB, PtrVCB->PtrStreamFileObject );


		if( PtrBitmapBCB )
		{
			CcUnpinData( PtrBitmapBCB );
			PtrBitmapBCB = NULL;
		}
		
		//
		//	Updating the Block count in the Group Descriptor...
		//	
		
		{
			PBCB					PtrDescriptorBCB = NULL;
			PEXT2_GROUP_DESCRIPTOR	PtrGroupDescriptor = NULL;
			ULONG					NumberOfBytesToRead = 0;
			//
			//	Updating the Free Blocks count in the Group Descriptor...
			//	
			PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeBlocksCount++;

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

			if (!CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrDescriptorBCB ,
				   (PVOID*)&PtrGroupDescriptor )) 
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
				//
				//	Ignore this error...
				//	Not fatal...
			}
			else
			{
				PtrGroupDescriptor[ GroupNo ].bg_free_blocks_count= 
					PtrVCB->PtrGroupDescriptors[ GroupNo ].FreeBlocksCount; 

				//
				//	Not synchronously flushing this information...
				//	Lazy writing will do...
				//
				CcSetDirtyPinnedData( PtrDescriptorBCB, NULL );
				CcUnpinData( PtrDescriptorBCB );
				PtrDescriptorBCB = NULL;
			}
		}

		//
		//	Update the Block count
		//	in the super block and in the VCB
		//
		{
			//	Ext2 Super Block information...
			PEXT2_SUPER_BLOCK	PtrSuperBlock = NULL;
			PBCB				PtrSuperBlockBCB = NULL;
			ULONG				NumberOfBytesToRead = 0;

			PtrVCB->FreeBlocksCount++;

			//	Reading in the super block...
			VolumeByteOffset.QuadPart = 1024;
			NumberOfBytesToRead = Ext2Align( sizeof( EXT2_SUPER_BLOCK ), LogicalBlockSize );

			if( !CcPinRead( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   NumberOfBytesToRead,
				   TRUE,
				   &PtrSuperBlockBCB,
				   (PVOID*)&PtrSuperBlock ) )
			{
				DebugTrace(DEBUG_TRACE_ERROR,   "Cache read failiure while reading in volume meta data", 0);
			}
			else
			{
				PtrSuperBlock->s_free_blocks_count = PtrVCB->FreeBlocksCount;
				CcSetDirtyPinnedData( PtrSuperBlockBCB, NULL );
				Ext2SaveBCB( PtrIrpContext, PtrSuperBlockBCB, PtrVCB->PtrStreamFileObject );
				CcUnpinData( PtrSuperBlockBCB );
				PtrSuperBlockBCB = NULL;
			}
		}
		try_exit:	NOTHING;
	}
	finally
	{
		if( PtrBitmapBCB )
		{
			CcUnpinData( PtrBitmapBCB );
			PtrBitmapBCB = NULL;
		}
	}
	return RC;
}

BOOLEAN NTAPI Ext2UpdateFileSize(	
	PtrExt2IrpContext	PtrIrpContext,
	PFILE_OBJECT		PtrFileObject,
	PtrExt2FCB			PtrFCB)
{
	EXT2_INODE			Inode;
	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;

	if( PtrFileObject->PrivateCacheMap )
	{
		CcSetFileSizes( PtrFileObject, (PCC_FILE_SIZES)&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize));
	}
	//	Now update the size on the disk...
	//	Read in the inode...
	if( ! NT_SUCCESS( Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		return FALSE;
	}

	Inode.i_size = PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.LowPart;
	//	Update time also???

	//	Updating the inode...
	if( NT_SUCCESS( Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*************************************************************************
*
* Function: Ext2DeleteFile()
*
* Description:
*	The functions will delete a file
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Return Value: Success / Failure...
*
*************************************************************************/
BOOLEAN NTAPI Ext2DeleteFile(
	PtrExt2FCB			PtrFCB,
	PtrExt2IrpContext	PtrIrpContext)
{
	EXT2_INODE			Inode;
	PtrExt2FCB			PtrParentFCB = NULL;
	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;
	
	//
	//	Get the Parent Directory...
	PtrParentFCB = Ext2LocateFCBInCore( PtrVCB, PtrFCB->ParentINodeNo );
	Ext2InitializeFCBInodeInfo( PtrFCB );
	
	//	1.
	//	Free up the directory entry...
	if( !Ext2FreeDirectoryEntry( PtrIrpContext,
			PtrParentFCB, &PtrFCB->FCBName->ObjectName ) )
	{
		return FALSE;
	}

	//	2. 
	//	Decrement Link count...
	if( !NT_SUCCESS( Ext2ReadInode( PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		return FALSE;
	}

	ASSERT( Inode.i_links_count == PtrFCB->LinkCount );
	
	Inode.i_links_count--;
	PtrFCB->LinkCount = Inode.i_links_count; 

	if( !Inode.i_links_count )
	{
		//
		//	Setting the deletion time field in the inode...
		//
		ULONG Time;
		Time = Ext2GetCurrentTime();
		Inode.i_dtime = Time ;
	}

	//	3. 
	//	Updating the inode...

	if( NT_SUCCESS( Ext2WriteInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo, &Inode ) ) )
	{
		if( Inode.i_links_count )
		{
			//	Some more links to the same file are available...
			//	So we won't deallocate the data blocks...
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}

	//	4.
	//	Free up the inode...
	Ext2DeallocInode( PtrIrpContext, PtrVCB, PtrFCB->INodeNo );

	//	5.
	//	Release the data blocks...
	Ext2ReleaseDataBlocks( PtrFCB, PtrIrpContext);

	return TRUE;
}


/*************************************************************************
*
* Function: Ext2ReleaseDataBlocks()
*
* Description:
*	The functions will release all the data blocks in a file
*	It does NOT update the file inode... 
*
* Expected Interrupt Level (for execution) :
*  IRQL_PASSIVE_LEVEL 
*
* Return Value: Success / Failure...
*
*************************************************************************/
BOOLEAN NTAPI Ext2ReleaseDataBlocks(
	PtrExt2FCB			PtrFCB,
	PtrExt2IrpContext	PtrIrpContext)
{
	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;
	ULONG LogicalBlockSize;
	ULONG i;


	LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

	//	Release the data blocks...

	//	1.
	//	Free up the triple indirect blocks...
	if( PtrFCB->IBlock[ EXT2_TIND_BLOCK ] )
	{
		
		PBCB			PtrSIBCB = NULL;
		PBCB			PtrDIBCB = NULL;
		PBCB			PtrTIBCB = NULL;

		ULONG *			PtrPinnedSIndirectBlock = NULL;
		ULONG *			PtrPinnedDIndirectBlock = NULL;
		ULONG *			PtrPinnedTIndirectBlock = NULL;
		
		LARGE_INTEGER	VolumeByteOffset;
		ULONG			TIndex, DIndex, SIndex;

		//	Pin the Double Indirect Pointer Block...
		VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_TIND_BLOCK ] * LogicalBlockSize;
		if (!CcMapData( PtrVCB->PtrStreamFileObject,
		   &VolumeByteOffset,
		   LogicalBlockSize,
		   TRUE,
		   &PtrTIBCB,
		   (PVOID*)&PtrPinnedTIndirectBlock )) 
		{
			return FALSE;
		}

		//	Read the Block numbers off the Triple Indirect Pointer Block...
		for( TIndex = 0; TIndex < (LogicalBlockSize/sizeof(ULONG)); TIndex++ )
		{
			if( PtrPinnedTIndirectBlock[ TIndex ] )
			{
				VolumeByteOffset.QuadPart = PtrPinnedTIndirectBlock[TIndex] * LogicalBlockSize;
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   LogicalBlockSize,
				   TRUE,
				   &PtrDIBCB,
				   (PVOID*)&PtrPinnedDIndirectBlock )) 
				{
					return FALSE;
				}

				//	Read the Block numbers off the Double Indirect Pointer Blocks...
				for( DIndex = 0; DIndex < (LogicalBlockSize/sizeof(ULONG)); DIndex++ )
				{
					if( PtrPinnedDIndirectBlock[DIndex] )
					{
						VolumeByteOffset.QuadPart = PtrPinnedDIndirectBlock[DIndex] * LogicalBlockSize;
						if (!CcMapData( PtrVCB->PtrStreamFileObject,
						   &VolumeByteOffset,
						   LogicalBlockSize,
						   TRUE,
						   &PtrSIBCB,
						   (PVOID*)&PtrPinnedSIndirectBlock )) 
						{
							return FALSE;
						}

						//	Read the Block numbers off the Single Indirect Pointer Blocks and 
						//	free the data blocks
						for( SIndex = 0; SIndex < (LogicalBlockSize/sizeof(ULONG)); SIndex++ )
						{
							if( PtrPinnedSIndirectBlock[ SIndex ] )
							{
								Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrPinnedSIndirectBlock[SIndex] );
							}
							else
							{
								break;
							}
						}
						CcUnpinData( PtrSIBCB );
						
						//	Deallocating
						//	Single Indirect Pointer Block
						Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrPinnedDIndirectBlock[DIndex] );
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
		CcUnpinData( PtrTIBCB );
		//	Deallocating Triple Indirect Pointer Blocks
		Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrFCB->IBlock[ EXT2_TIND_BLOCK ] );
	}

	//	2.
	//	Free up the double indirect blocks...
	if( PtrFCB->IBlock[ EXT2_DIND_BLOCK ] )
	{
		PBCB			PtrDIBCB = NULL;
		PBCB			PtrSIBCB = NULL;
		ULONG *			PtrPinnedSIndirectBlock = NULL;
		ULONG *			PtrPinnedDIndirectBlock = NULL;
		
		LARGE_INTEGER	VolumeByteOffset;
		ULONG			DIndex, SIndex;

		//	Pin the Double Indirect Pointer Block...
		VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_DIND_BLOCK ] * LogicalBlockSize;
		if (!CcMapData( PtrVCB->PtrStreamFileObject,
		   &VolumeByteOffset,
		   LogicalBlockSize,
		   TRUE,
		   &PtrDIBCB,
		   (PVOID*)&PtrPinnedDIndirectBlock )) 
		{
			return FALSE;
		}

		//	Read the Block numbers off the Double Indirect Pointer Block...
		for( DIndex = 0; DIndex < (LogicalBlockSize/sizeof(ULONG)); DIndex++ )
		{
			if( PtrPinnedDIndirectBlock[DIndex] )
			{
				VolumeByteOffset.QuadPart = PtrPinnedDIndirectBlock[DIndex] * LogicalBlockSize;
				if (!CcMapData( PtrVCB->PtrStreamFileObject,
				   &VolumeByteOffset,
				   LogicalBlockSize,
				   TRUE,
				   &PtrSIBCB,
				   (PVOID*)&PtrPinnedSIndirectBlock )) 
				{
					return FALSE;
				}

				//	Read the Block numbers off the Single Indirect Pointer Blocks and 
				//	free the data blocks
				for( SIndex = 0; SIndex < (LogicalBlockSize/sizeof(ULONG)); SIndex++ )
				{
					if( PtrPinnedSIndirectBlock[ SIndex ] )
					{
						Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrPinnedSIndirectBlock[SIndex] );
					}
					else
					{
						break;
					}
				}
				CcUnpinData( PtrSIBCB );
				
				//	Deallocating
				//	Single Indirect Pointer Block
				Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrPinnedDIndirectBlock[DIndex] );
			}
			else
			{
				break;
			}
		}
		CcUnpinData( PtrDIBCB );
		//	Deallocating Double Indirect Pointer Blocks
		Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrFCB->IBlock[ EXT2_DIND_BLOCK ] );
	}

	//	3.
	//	Free up the single indirect blocks...
	if( PtrFCB->IBlock[ EXT2_IND_BLOCK ] )
	{
		PBCB			PtrBCB = NULL;
		ULONG *			PtrPinnedSIndirectBlock = NULL;
		LARGE_INTEGER	VolumeByteOffset;
		ULONG			Index;

		//	Pin the Single Indirect Pointer Block...
		VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_IND_BLOCK ] * LogicalBlockSize;
		if (!CcMapData( PtrVCB->PtrStreamFileObject,
		   &VolumeByteOffset,
		   LogicalBlockSize,
		   TRUE,
		   &PtrBCB,
		   (PVOID*)&PtrPinnedSIndirectBlock )) 
		{
			return FALSE;
		}

		//	Read the Block numbers off the Indirect Pointer Block and 
		//	free the data blocks
		for( Index = 0; Index < (LogicalBlockSize/sizeof(ULONG)); Index++ )
		{
			if( PtrPinnedSIndirectBlock[Index] )
			{
				Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrPinnedSIndirectBlock[Index] );
			}
			else
			{
				break;
			}
		}
		CcUnpinData( PtrBCB );
		Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrFCB->IBlock[ EXT2_IND_BLOCK ] );
	}

	//	4.
	//	Free up the direct blocks...
	for( i = 0; i < EXT2_NDIR_BLOCKS; i++ )
	{
		if( PtrFCB->IBlock[ i ] )
		{
			Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrFCB->IBlock[ i ] );
		}
		else
		{
			break;
		}
	}
	return TRUE;
}


BOOLEAN NTAPI Ext2TruncateFileAllocationSize(
	PtrExt2IrpContext	PtrIrpContext,
	PtrExt2FCB			PtrFCB,
	PFILE_OBJECT		PtrFileObject,
	PLARGE_INTEGER		PtrAllocationSize )
{
	PtrExt2VCB			PtrVCB = PtrFCB->PtrVCB;
	ULONG LogicalBlockSize;
	ULONG i;

	ULONG NoOfBlocksToBeLeft= 0;
	ULONG CurrentBlockNo = 0;

	//
	//	This function has not been tested...
	//
	Ext2BreakPoint();

	LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;
	NoOfBlocksToBeLeft = (ULONG) (PtrAllocationSize->QuadPart / LogicalBlockSize);

	

	//	Release the data blocks...

	//	1.
	//	Free up the direct blocks...
	for( i = NoOfBlocksToBeLeft; i < EXT2_NDIR_BLOCKS; i++ )
	{
		if( PtrFCB->IBlock[ i ] )
		{
			Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrFCB->IBlock[ i ] );
			PtrFCB->IBlock[ i ] = 0;
		}
		else
		{
			break;
		}
	}

	//	2.
	//	Free up the single indirect blocks...
	CurrentBlockNo = EXT2_NDIR_BLOCKS;

	if( PtrFCB->IBlock[ EXT2_IND_BLOCK ] )
	{
		PBCB			PtrBCB = NULL;
		ULONG *			PtrPinnedSIndirectBlock = NULL;
		LARGE_INTEGER	VolumeByteOffset;
		ULONG			Index;

		//	Pin the Single Indirect Pointer Block...
		VolumeByteOffset.QuadPart = PtrFCB->IBlock[ EXT2_IND_BLOCK ] * LogicalBlockSize;
		if (!CcMapData( PtrVCB->PtrStreamFileObject,
		   &VolumeByteOffset,
		   LogicalBlockSize,
		   TRUE,
		   &PtrBCB,
		   (PVOID*)&PtrPinnedSIndirectBlock )) 
		{
			return FALSE;
		}

		//	Read the Block numbers off the Indirect Pointer Block and 
		//	free the data blocks
		for( Index = 0; Index < (LogicalBlockSize/sizeof(ULONG)); 
			 Index++, CurrentBlockNo++ )
		{
			if( CurrentBlockNo >= NoOfBlocksToBeLeft )
			{
				if( PtrPinnedSIndirectBlock[Index] )
				{
					Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrPinnedSIndirectBlock[Index] );
				}
				else
				{
					break;
				}
			}
			else if( !PtrPinnedSIndirectBlock[Index] )
			{
				break;
			}
		}
		if( NoOfBlocksToBeLeft <= EXT2_NDIR_BLOCKS )
		{
			Ext2DeallocBlock( PtrIrpContext, PtrVCB, PtrFCB->IBlock[ EXT2_IND_BLOCK ] );
			PtrFCB->IBlock[ EXT2_IND_BLOCK ] = 0;
		}

		CcUnpinData( PtrBCB );
	}
	
	//	3.
	//	Free up the double indirect blocks...
	if( PtrFCB->IBlock[ EXT2_DIND_BLOCK ] )
	{
		
	}

	//	4.
	//	Free up the triple indirect blocks...
	if( PtrFCB->IBlock[ EXT2_TIND_BLOCK ] )
	{
		
	}

	return TRUE;
}

BOOLEAN NTAPI Ext2IsDirectoryEmpty(
	PtrExt2FCB			PtrFCB,
	PtrExt2CCB			PtrCCB,
	PtrExt2IrpContext	PtrIrpContext)
{

	PFILE_OBJECT		PtrFileObject = NULL;

	if( !Ext2IsFlagOn(PtrFCB->FCBFlags, EXT2_FCB_DIRECTORY) )
	{
		return FALSE;
	}

	//	1. 
	//	Initialize the Blocks in the FCB...
	//
	Ext2InitializeFCBInodeInfo( PtrFCB );

	
	//	2.
	//	Get hold of the file object...
	//
	PtrFileObject = PtrCCB->PtrFileObject;


	//	3.
	//	Now initiating Caching, pinned access to be precise ...
	//
	if (PtrFileObject->PrivateCacheMap == NULL) 
	{
		CcInitializeCacheMap(PtrFileObject, (PCC_FILE_SIZES)(&(PtrFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize)),
			TRUE,		// We utilize pin access for directories
			&(Ext2GlobalData.CacheMgrCallBacks), // callbacks
			PtrFCB );		// The context used in callbacks
	}

	//	4.
	//	Getting down to the real business now... ;)
	//	Read in the directory contents and do a search 
	//
	{
		LARGE_INTEGER	StartBufferOffset;
		ULONG			PinBufferLength;
		ULONG			BufferIndex;
		PBCB			PtrBCB = NULL;
		BYTE *			PtrPinnedBlockBuffer = NULL;
		PEXT2_DIR_ENTRY	PtrDirEntry = NULL;
		BOOLEAN			Found = FALSE;

		StartBufferOffset.QuadPart = 0;

		//
		//	Read in the whole directory
		//	**Bad programming**
		//	Will do for now.
		//
		PinBufferLength = PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.LowPart;
		if (!CcMapData( PtrFileObject,
                  &StartBufferOffset,
                  PinBufferLength,
                  TRUE,
                  &PtrBCB,
                  (PVOID*)&PtrPinnedBlockBuffer ) )
		{
			return FALSE;
		}
		
		//
		//	Walking through now...
 		//
		for( BufferIndex = 0, Found = FALSE; !Found && BufferIndex < ( PtrFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) ; BufferIndex += PtrDirEntry->rec_len )
		{
			PtrDirEntry = (PEXT2_DIR_ENTRY) &PtrPinnedBlockBuffer[ BufferIndex ];
			if( PtrDirEntry->inode == 0)
			{
				//	Deleted entry...
				//  Ignore...
				continue;
			}
			if( PtrDirEntry->name[0] == '.' )
			{
				if( PtrDirEntry->name_len == 1 || 
				  ( PtrDirEntry->name_len == 2 && PtrDirEntry->name[1] == '.' ) )
				{
				  continue;
				}
			}
			Found = TRUE;
		}
		CcUnpinData( PtrBCB );
		PtrBCB = NULL;

		return !Found;
	}
}


NTSTATUS NTAPI Ext2RenameOrLinkFile( 
	PtrExt2FCB					PtrSourceFCB, 
	PFILE_OBJECT				PtrSourceFileObject,	
	PtrExt2IrpContext			PtrIrpContext,
	PIRP						PtrIrp, 
	PFILE_RENAME_INFORMATION	PtrRenameInfo)
{
	PtrExt2FCB				PtrParentFCB = NULL;
	PtrExt2VCB				PtrSourceVCB = PtrSourceFCB->PtrVCB;

	PtrExt2FCB				PtrTargetFCB = NULL;
	PtrExt2CCB				PtrTargetCCB = NULL;
	PtrExt2VCB				PtrTargetVCB = NULL;


	FILE_INFORMATION_CLASS	FunctionalityRequested;
	PIO_STACK_LOCATION		PtrIoStackLocation = NULL;
	PFILE_OBJECT			TargetFileObject = NULL;
	BOOLEAN					ReplaceExistingFile = FALSE;
	BOOLEAN					Found = FALSE;

	PtrIoStackLocation = IoGetCurrentIrpStackLocation(PtrIrp);
	FunctionalityRequested = PtrIoStackLocation->Parameters.SetFile.FileInformationClass;
	TargetFileObject = PtrIoStackLocation->Parameters.SetFile.FileObject;
	ReplaceExistingFile = PtrIoStackLocation->Parameters.SetFile.ReplaceIfExists;

	// Get the FCB and CCB pointers
	Ext2GetFCB_CCB_VCB_FromFileObject ( 
		TargetFileObject , &PtrTargetFCB, &PtrTargetCCB, &PtrTargetVCB);

	if( !PtrTargetCCB )
	{
		return STATUS_ACCESS_DENIED;
	}
	if( PtrTargetVCB != PtrSourceVCB )
	{
		//	Cannot rename across volumes...
		return STATUS_ACCESS_DENIED;
	}
	if ( !Ext2IsFlagOn( PtrTargetFCB->FCBFlags, EXT2_FCB_DIRECTORY ) )
	{
		//	Target has to be a folder...
		return STATUS_ACCESS_DENIED;
	}

	//	1.
	//	Open the parent folder...
	PtrParentFCB = Ext2LocateFCBInCore( PtrSourceVCB, PtrSourceFCB->ParentINodeNo );
	if( !PtrParentFCB )
	{
		//	Get the folder from the disk
		//	Use the inode no PtrSourceFCB->ParentINodeNo
		//
		//	For now...
		return STATUS_ACCESS_DENIED;
	}

	//	2.
	//	Check if the file exists in the TargetFolder...
	{
		LARGE_INTEGER	StartBufferOffset;
		ULONG			PinBufferLength;
		ULONG			BufferIndex;
		PBCB			PtrBCB = NULL;
		BYTE *			PtrPinnedBlockBuffer = NULL;
		PEXT2_DIR_ENTRY	PtrDirEntry = NULL;
		int				i;

		StartBufferOffset.QuadPart = 0;

		//
		//	Read in the whole directory
		//
		if ( TargetFileObject->PrivateCacheMap == NULL )
		{
			CcInitializeCacheMap(
				TargetFileObject, 
				(PCC_FILE_SIZES)(&(PtrTargetFCB->NTRequiredFCB.CommonFCBHeader.AllocationSize)),
				TRUE,									// We utilize pin access for directories
				&(Ext2GlobalData.CacheMgrCallBacks),	// callbacks
				PtrTargetCCB );							// The context used in callbacks
		}
		
		PinBufferLength = PtrTargetFCB->NTRequiredFCB.CommonFCBHeader.FileSize.LowPart;
		if (!CcMapData( TargetFileObject,
                  &StartBufferOffset,
                  PinBufferLength,
                  TRUE,
                  &PtrBCB,
                  (PVOID*)&PtrPinnedBlockBuffer ) )
		{
			return FALSE;
		}
		
		//
		//	Walking through now...
 		//
		for( BufferIndex = 0, Found = FALSE; !Found && BufferIndex < ( PtrTargetFCB->NTRequiredFCB.CommonFCBHeader.FileSize.QuadPart - 1) ; BufferIndex += PtrDirEntry->rec_len )
		{
			PtrDirEntry = (PEXT2_DIR_ENTRY) &PtrPinnedBlockBuffer[ BufferIndex ];
			if( PtrDirEntry->inode == 0)
			{
				//	Deleted entry...
				//  Ignore...
				continue;
			}
			if( PtrDirEntry->name_len == (PtrTargetCCB->RenameLinkTargetFileName.Length/2) )
			{
				Found = TRUE;
				for( i =0; i < PtrDirEntry->name_len ; i++ )
				{
					if( PtrDirEntry->name[i] != PtrTargetCCB->RenameLinkTargetFileName.Buffer[i] )
					{
						Found = FALSE;
						break;
					}
				}
			}
		}
		CcUnpinData( PtrBCB );
		PtrBCB = NULL;
	}

	//	3.
	//	If the file exists, delete it if requested..
	if( Found )
	{
		if( !ReplaceExistingFile )
		{
			return STATUS_OBJECT_NAME_COLLISION;
		}
		//	Delete the file...
		//	Reject this for now...
		return STATUS_ACCESS_DENIED;
	}


	{
		ULONG	Type = EXT2_FT_REG_FILE;
		if( Ext2IsFlagOn( PtrSourceFCB->FCBFlags, EXT2_FCB_DIRECTORY ) )
		{
			Type = EXT2_FT_DIR;
		}

		ASSERT( TargetFileObject );

		//	4.
		//	Remove the old entry...
		Ext2FreeDirectoryEntry(	PtrIrpContext, PtrParentFCB,
				&PtrSourceFCB->FCBName->ObjectName);

		//	5. 
		//	Create a new entry...
		Ext2MakeNewDirectoryEntry(
			PtrIrpContext,				//	This IRP Context
			PtrTargetFCB,				//	Parent Folder FCB
			TargetFileObject,			//	Parent Folder Object
			&PtrTargetCCB->RenameLinkTargetFileName, //	New entry's name
			Type,						//	The type of the new entry
			PtrSourceFCB->INodeNo );	//	The inode no of the new entry...

	}

	//	6. 
	//	Update the PtrSourceFCB...
	{

		PtrExt2ObjectName		PtrObjectName;
		if( PtrSourceFCB->FCBName )
		{
			Ext2ReleaseObjectName( PtrSourceFCB->FCBName );
		}
		PtrObjectName = Ext2AllocateObjectName();
		Ext2CopyUnicodeString( &PtrObjectName->ObjectName, &PtrTargetCCB->RenameLinkTargetFileName ); 
		PtrSourceFCB->FCBName = PtrObjectName;
		PtrSourceFCB->ParentINodeNo = PtrTargetFCB->INodeNo;
	}

	if( PtrTargetCCB->RenameLinkTargetFileName.Length )
	{
		Ext2DeallocateUnicodeString( &PtrTargetCCB->RenameLinkTargetFileName );
	}

	return STATUS_SUCCESS;
}

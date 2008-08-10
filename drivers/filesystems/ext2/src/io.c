/*************************************************************************
*
* File: io.c
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	This file contains low level disk io routines.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#include			"ext2fsd.h"

// define the file specific bug-check id
#define			EXT2_BUG_CHECK_ID				EXT2_FILE_IO

/*************************************************************************
*
* Function: Ext2PassDownMultiReadWriteIRP()
*
* Description:
*	pass down multiple read IRPs as Associated IRPs
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: STATUS_SUCCESS / STATUS_PENDING / Error
*
*************************************************************************/
NTSTATUS NTAPI Ext2PassDownMultiReadWriteIRP( 
	PEXT2_IO_RUN			PtrIoRuns, 
	UINT					Count, 
	ULONG					TotalReadWriteLength,
	PtrExt2IrpContext		PtrIrpContext,
	PtrExt2FCB				PtrFCB,
	BOOLEAN					SynchronousIo)
{
	PIRP				PtrMasterIrp;
	PIRP				PtrAssociatedIrp;
    PIO_STACK_LOCATION	PtrIrpSp;
    PMDL				PtrMdl;
	PtrExt2VCB			PtrVCB;
	UINT				i;
	ULONG				BufferOffset;
	PEXT2_IO_CONTEXT	PtrIoContext = NULL;
	PKEVENT				PtrSyncEvent = NULL;
	ULONG				LogicalBlockSize;
	ULONG				ReadWriteLength;

	NTSTATUS RC = STATUS_SUCCESS;

	PtrVCB = PtrFCB->PtrVCB;
	PtrMasterIrp = PtrIrpContext->Irp;
	LogicalBlockSize = EXT2_MIN_BLOCK_SIZE << PtrVCB->LogBlockSize;

	try
	{
		if( !SynchronousIo )
		{
			IoMarkIrpPending( PtrIrpContext->Irp );
			//	We will be returning STATUS_PENDING...
		}

		if( !PtrMasterIrp->MdlAddress )
		{
			Ext2LockCallersBuffer( PtrMasterIrp, TRUE, TotalReadWriteLength );
		}

		if( SynchronousIo )
		{
			PtrSyncEvent = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( sizeof(KEVENT) )  );
			if ( !PtrSyncEvent )
			{
				RC = STATUS_INSUFFICIENT_RESOURCES;
				try_return ( RC );
			}
			KeInitializeEvent( PtrSyncEvent, SynchronizationEvent, FALSE );
		}
		//
		//	Allocate and initialize a completion context
		//
		PtrIoContext = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( sizeof(EXT2_IO_CONTEXT) )  );
		if ( !PtrIoContext )
		{
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return ( RC );
		}

		RtlZeroMemory( PtrIoContext, sizeof(EXT2_IO_CONTEXT) );
		PtrIoContext->Count = Count;
		PtrIoContext->NodeIdentifier.NodeType = EXT2_NODE_TYPE_IO_CONTEXT;
		PtrIoContext->NodeIdentifier.NodeSize = sizeof( EXT2_IO_CONTEXT );
		PtrIoContext->PtrMasterIrp = PtrMasterIrp;
		PtrIoContext->PtrSyncEvent = PtrSyncEvent;
		PtrIoContext->ReadWriteLength = TotalReadWriteLength;

		

		for( ReadWriteLength = 0, BufferOffset = 0, i = 0; i < Count; i++, BufferOffset += ReadWriteLength )
		{
			
			ReadWriteLength = PtrIoRuns[ i].EndOffset - PtrIoRuns[ i].StartOffset;
			
			//
			//	Allocating an Associated IRP...
			//
			PtrAssociatedIrp = IoMakeAssociatedIrp( PtrMasterIrp,
					(CCHAR) (PtrVCB->TargetDeviceObject->StackSize + 1 ) );
			PtrIoRuns[ i].PtrAssociatedIrp = PtrAssociatedIrp;
			ASSERT ( PtrAssociatedIrp );
			PtrMasterIrp->AssociatedIrp.IrpCount ++;
			
			//
			//	Allocating a Memory Descriptor List...
			//
			PtrMdl = IoAllocateMdl( (PCHAR) PtrMasterIrp->UserBuffer + BufferOffset, //	Virtual Address
				ReadWriteLength,	FALSE, FALSE, PtrAssociatedIrp );
			
			//
			//	and building a partial MDL...
			//
			IoBuildPartialMdl( PtrMasterIrp->MdlAddress,
				PtrMdl, (PCHAR)PtrMasterIrp->UserBuffer + BufferOffset, ReadWriteLength );

			//
			//	Create an Irp stack location for ourselves...
			//
			IoSetNextIrpStackLocation( PtrAssociatedIrp );
			PtrIrpSp = IoGetCurrentIrpStackLocation( PtrAssociatedIrp );

			//
			//  Setup the Stack location to describe our read.
			//
			PtrIrpSp->MajorFunction = PtrIrpContext->MajorFunction;
			if( PtrIrpContext->MajorFunction == IRP_MJ_READ )
			{
				PtrIrpSp->Parameters.Read.Length = ReadWriteLength;
				PtrIrpSp->Parameters.Read.ByteOffset.QuadPart = 
					PtrIoRuns[i].LogicalBlock * ( LogicalBlockSize );
			}
			else if( PtrIrpContext->MajorFunction == IRP_MJ_WRITE )
			{
				PtrIrpSp->Parameters.Write.Length = ReadWriteLength;
				PtrIrpSp->Parameters.Write.ByteOffset.QuadPart = 
					PtrIoRuns[i].LogicalBlock * ( LogicalBlockSize );
			}

			//	PtrIrpSp->Parameters.Read.Length = ReadWriteLength;
			//	PtrIrpSp->Parameters.Read.ByteOffset.QuadPart = PtrIoRuns[i].LogicalBlock;


			//
			//	Setup a completion routine...
			//
			IoSetCompletionRoutine( PtrAssociatedIrp, 
						SynchronousIo ?	
						Ext2MultiSyncCompletionRoutine : 
						Ext2MultiAsyncCompletionRoutine,
						PtrIoContext, TRUE, TRUE, TRUE );

			//
			//	Initialise the next stack location for the driver below us to use...
			//
			PtrIrpSp = IoGetNextIrpStackLocation( PtrAssociatedIrp );
			PtrIrpSp->MajorFunction = PtrIrpContext->MajorFunction;
			if( PtrIrpContext->MajorFunction == IRP_MJ_READ )
			{
				PtrIrpSp->Parameters.Read.Length = ReadWriteLength;
				PtrIrpSp->Parameters.Read.ByteOffset.QuadPart = PtrIoRuns[i].LogicalBlock * ( LogicalBlockSize );
			}
			else if( PtrIrpContext->MajorFunction == IRP_MJ_WRITE )
			{
				PtrIrpSp->Parameters.Write.Length = ReadWriteLength;
				PtrIrpSp->Parameters.Write.ByteOffset.QuadPart = PtrIoRuns[i].LogicalBlock * ( LogicalBlockSize );
			}

			//	PtrIrpSp->Parameters.Read.Length = ReadWriteLength;
			//	PtrIrpSp->Parameters.Read.ByteOffset.QuadPart = 
			//	 	PtrIoRuns[i].LogicalBlock * ( LogicalBlockSize );
		}

		for( i = 0; i < Count; i++ ) {
                    DbgPrint("PASSING DOWN IRP %d TO TARGET DEVICE\n", i);
                    IoCallDriver( PtrVCB->TargetDeviceObject, PtrIoRuns[ i].PtrAssociatedIrp );
                }

		if( SynchronousIo )
		{
			//
			//	Synchronous IO 
			//	Wait for the IO to complete...
			//
                    DbgPrint("DEADLY WAIT (%d)\n", KeGetCurrentIrql());
			KeWaitForSingleObject( PtrSyncEvent,
				Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );
                        DbgPrint("DEADLY WAIT DONE\n");
			try_return ( RC );
		}
		else
		{
			//	Asynchronous IO...
			RC = STATUS_PENDING;
			try_return ( RC );
		}
	
		try_exit:	NOTHING;
	}
	finally 
	{
		if( PtrSyncEvent )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrSyncEvent );
			ExFreePool( PtrSyncEvent );
		}
		if( PtrIoContext && ! ( RC == STATUS_PENDING || RC == STATUS_SUCCESS ) )
		{
			//
			//	This means we are getting out of 
			//	this function without doing a read
			//	due to an error, maybe...
			//
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrIoContext);
			ExFreePool( PtrIoContext );
		}
	}
	return(RC);
}

NTSTATUS NTAPI Ext2PassDownSingleReadWriteIRP(
	PtrExt2IrpContext	PtrIrpContext,
	PIRP				PtrIrp, 
	PtrExt2VCB			PtrVCB,
	LARGE_INTEGER		ByteOffset, 
	uint32				ReadWriteLength, 
	BOOLEAN				SynchronousIo)
{
	NTSTATUS				RC = STATUS_SUCCESS;

	PEXT2_IO_CONTEXT		PtrIoContext = NULL;
	PKEVENT					PtrSyncEvent = NULL;

	PIO_STACK_LOCATION	PtrIrpNextSp = NULL;

	try
	{
		if( !PtrIrp->MdlAddress )
		{
			Ext2LockCallersBuffer( PtrIrp, TRUE, ReadWriteLength );
		}


		if( SynchronousIo )
		{
			PtrSyncEvent = Ext2AllocatePool( NonPagedPool, Ext2QuadAlign( sizeof(KEVENT) )  );
			if ( !PtrSyncEvent )
			{
				RC = STATUS_INSUFFICIENT_RESOURCES;
				try_return ( RC );
			}
			KeInitializeEvent( PtrSyncEvent, SynchronizationEvent, FALSE );
		}

		//
		//	Allocate and initialize a completion context
		//
		PtrIoContext = Ext2AllocatePool(NonPagedPool, Ext2QuadAlign( sizeof(EXT2_IO_CONTEXT) )  );
		if ( !PtrIoContext )
		{
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return ( RC );
		}

		RtlZeroMemory( PtrIoContext, sizeof(EXT2_IO_CONTEXT) );
		PtrIoContext->Count = 1;
		PtrIoContext->NodeIdentifier.NodeType = EXT2_NODE_TYPE_IO_CONTEXT;
		PtrIoContext->NodeIdentifier.NodeSize = sizeof( EXT2_IO_CONTEXT );
		PtrIoContext->PtrMasterIrp = NULL;
		PtrIoContext->PtrSyncEvent = PtrSyncEvent;
		PtrIoContext->ReadWriteLength = ReadWriteLength;

 		IoSetCompletionRoutine( PtrIrp, 
			SynchronousIo ?	
			Ext2SingleSyncCompletionRoutine: 
			Ext2SingleAsyncCompletionRoutine,
			PtrIoContext, TRUE, TRUE, TRUE );

		//
		//  Setup the next IRP stack location in the associated Irp for the disk
		//  driver beneath us.
		//
		PtrIrpNextSp = IoGetNextIrpStackLocation( PtrIrp );

		//
		//  Setup the Stack location to do a read from the disk driver.
		//
		PtrIrpNextSp->MajorFunction = PtrIrpContext->MajorFunction;
		if( PtrIrpContext->MajorFunction == IRP_MJ_READ )
		{
			PtrIrpNextSp->Parameters.Read.Length = ReadWriteLength;
			PtrIrpNextSp->Parameters.Read.ByteOffset = ByteOffset;
		}
		else if( PtrIrpContext->MajorFunction == IRP_MJ_WRITE )
		{
			PtrIrpNextSp->Parameters.Write.Length = ReadWriteLength;
			PtrIrpNextSp->Parameters.Write.ByteOffset = ByteOffset;
		}
		//
		//  Issue the read / write request
		//
		RC = IoCallDriver(PtrVCB->TargetDeviceObject, PtrIrp);

		if( SynchronousIo )
		{
			//
			//	Wait for completion...
			//
			RC = KeWaitForSingleObject( &PtrIoContext->PtrSyncEvent,
						Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

			RC = STATUS_SUCCESS;
		}
		else
		{
			RC = STATUS_PENDING;
		}

		try_exit:	NOTHING;
	}
	finally 
	{
		if( PtrSyncEvent )
		{
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrSyncEvent );
			ExFreePool( PtrSyncEvent );
		}
		if( PtrIoContext && !( RC == STATUS_PENDING || RC == STATUS_SUCCESS ) )
		{
			//
			//	This means we are getting out of 
			//	this function without doing a read / write
			//	due to an error, maybe...
			//
			DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrIoContext );
			ExFreePool( PtrIoContext );
		}
	}
	return RC;
}


/*************************************************************************
*
* Function: Ext2SingleSyncCompletionRoutine()
*
* Description:
*	Synchronous I/O Completion Routine
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: NTSTATUS - STATUS_SUCCESS(always)
*
*************************************************************************/
NTSTATUS NTAPI Ext2SingleSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
	PEXT2_IO_CONTEXT PtrContext = Contxt;

	if( Irp->PendingReturned  )
		IoMarkIrpPending( Irp );
	
	ASSERT( PtrContext );
	ASSERT( PtrContext->NodeIdentifier.NodeType == EXT2_NODE_TYPE_IO_CONTEXT );

	KeSetEvent( PtrContext->PtrSyncEvent, 0, FALSE );
	DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrContext );
	ExFreePool( PtrContext );

	return STATUS_SUCCESS;
}

/*************************************************************************
*
* Function: Ext2SingleAsyncCompletionRoutine()
*
* Description:
*	Asynchronous I/O Completion Routine
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: NTSTATUS - STATUS_SUCCESS(always)
*
*************************************************************************/
NTSTATUS NTAPI Ext2SingleAsyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
	PEXT2_IO_CONTEXT PtrContext = Contxt;

	if( Irp->PendingReturned  )
		IoMarkIrpPending( Irp );
	
	ASSERT( PtrContext );
	ASSERT( PtrContext->NodeIdentifier.NodeType == EXT2_NODE_TYPE_IO_CONTEXT );

	DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrContext );
	ExFreePool( PtrContext );

	return STATUS_SUCCESS;
}

/*************************************************************************
*
* Function: Ext2MultiSyncCompletionRoutine()
*
* Description:
*	Synchronous I/O Completion Routine
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: NTSTATUS - STATUS_SUCCESS(always)
*
*************************************************************************/
NTSTATUS NTAPI Ext2MultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{

	PEXT2_IO_CONTEXT PtrContext = Contxt;
	ASSERT( PtrContext );

	if( Irp->PendingReturned )
	{
		IoMarkIrpPending( Irp );
	}

	if (!NT_SUCCESS( Irp->IoStatus.Status )) 
	{
		PtrContext->PtrMasterIrp->IoStatus.Status = Irp->IoStatus.Status;
    }

    if (InterlockedDecrement( &PtrContext->Count ) == 0)
	{
		if ( NT_SUCCESS( PtrContext->PtrMasterIrp->IoStatus.Status ) )
		{
			PtrContext->PtrMasterIrp->IoStatus.Information = PtrContext->ReadWriteLength;
		}
		else
		{
			PtrContext->PtrMasterIrp->IoStatus.Information = 0;
		}

        KeSetEvent( PtrContext->PtrSyncEvent, 0, FALSE );
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrContext );
		ExFreePool( PtrContext );
    }

	//
	//	The master Irp will be automatically completed 
	//	when all the associated IRPs are completed
	//
	return STATUS_SUCCESS;
}

/*************************************************************************
*
* Function: Ext2MultiAsyncCompletionRoutine()
*
* Description:
*	Asynchronous I/O Completion Routine
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: NTSTATUS - STATUS_SUCCESS(always)
*
*************************************************************************/
NTSTATUS NTAPI Ext2MultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{

	PEXT2_IO_CONTEXT PtrContext = Contxt;
	ASSERT( PtrContext );
	
	if( Irp->PendingReturned )
	{
		IoMarkIrpPending( Irp );
	}

	if (!NT_SUCCESS( Irp->IoStatus.Status )) 
	{
		PtrContext->PtrMasterIrp->IoStatus.Status = Irp->IoStatus.Status;
    }

    if (InterlockedDecrement( &PtrContext->Count ) == 0)
	{
		if ( NT_SUCCESS( PtrContext->PtrMasterIrp->IoStatus.Status ) )
		{
			PtrContext->PtrMasterIrp->IoStatus.Information = PtrContext->ReadWriteLength;
		}
		else
		{
			PtrContext->PtrMasterIrp->IoStatus.Information = 0;
		}
		DebugTrace( DEBUG_TRACE_FREE, "Freeing  = %lX [io]", PtrContext );
		ExFreePool( PtrContext );
    }

	//
	//	The master Irp will be automatically completed 
	//	when all the associated IRPs are completed
	//	Returning STATUS_SUCCESS to continue postprocessing...
	//
	return STATUS_SUCCESS;
}

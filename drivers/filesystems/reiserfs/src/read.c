/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             read.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

NTSTATUS
RfsdReadComplete (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdReadFile (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdReadVolume (IN PRFSD_IRP_CONTEXT IrpContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdCompleteIrpContext)
#pragma alloc_text(PAGE, RfsdCopyRead)
#pragma alloc_text(PAGE, RfsdRead)
#pragma alloc_text(PAGE, RfsdReadVolume)
#pragma alloc_text(PAGE, RfsdReadInode)
#pragma alloc_text(PAGE, RfsdReadFile)
#pragma alloc_text(PAGE, RfsdReadComplete)
#endif

/* FUNCTIONS *************************************************************/

/** Proxy to CcCopyRead, which simply asserts the success of the IoStatus. */
BOOLEAN 
RfsdCopyRead(
    IN PFILE_OBJECT  FileObject,
    IN PLARGE_INTEGER  FileOffset,
    IN ULONG  Length,
    IN BOOLEAN  Wait,
    OUT PVOID  Buffer,
    OUT PIO_STATUS_BLOCK  IoStatus
    )
{
    BOOLEAN bRet;

    PAGED_CODE();

    bRet=  CcCopyRead(FileObject,
                FileOffset,
                Length,
                Wait,
                Buffer,
                IoStatus    );

    if (bRet) {
        ASSERT(NT_SUCCESS(IoStatus->Status));
    }

    return bRet;
/*
    PVOID Bcb = NULL;
    PVOID Buf = NULL;

    if (CcMapData(  FileObject,
                    FileOffset,
                    Length,
                    Wait,
                    &Bcb,
                    &Buf    )) {
        RtlCopyMemory(Buffer,  Buf, Length);
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = Length;
        CcUnpinData(Bcb);
        return TRUE;

    } else {
        // IoStatus->Status = STATUS_
        return FALSE;
    }
*/
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdReadVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB           Vcb = 0;
    PRFSD_CCB           Ccb;
    PRFSD_FCBVCB        FcbOrVcb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer = NULL;
    PRFSD_BDL           rfsd_bdl = NULL;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        FileObject = IrpContext->FileObject;

        FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;
        
        ASSERT(FcbOrVcb);
        
        if (!(FcbOrVcb->Identifier.Type == RFSDVCB && (PVOID)FcbOrVcb == (PVOID)Vcb)) {

            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
            
        Irp->IoStatus.Information = 0;

        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
            
        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
            
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);
            
        if (Length == 0) {

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (Ccb != NULL) {

            if(!IsFlagOn(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO)) {
                if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
                    Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
                }
            }

            {
                RFSD_BDL BlockArray;

                if ((ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
                   (Length & (SECTOR_SIZE - 1)) ) {
                    Status = STATUS_INVALID_PARAMETER;
                    _SEH2_LEAVE;
                }

                Status = RfsdLockUserBuffer(
                    IrpContext->Irp,
                    Length,
                    IoReadAccess );
                
                if (!NT_SUCCESS(Status)) {
                    _SEH2_LEAVE;
                }

                BlockArray.Irp = NULL;
                BlockArray.Lba = ByteOffset.QuadPart;;
                BlockArray.Offset = 0;
                BlockArray.Length = Length;

                Status = RfsdReadWriteBlocks(IrpContext,
                                    Vcb,
                                    &BlockArray,
                                    Length,
                                    1,
                                    FALSE   );
                Irp = IrpContext->Irp;

                _SEH2_LEAVE;
            }
        }
            
        if (Nocache &&
            ( (ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
              (Length & (SECTOR_SIZE - 1)) )) {
            DbgBreak();

            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        
        if (!PagingIo) {
            if (!ExAcquireResourceSharedLite(
                &Vcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            MainResourceAcquired = TRUE;

        } else {

            if (!ExAcquireResourceSharedLite(
                &Vcb->PagingIoResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            PagingIoResourceAcquired = TRUE;
        }
        
    
        if (ByteOffset.QuadPart >=
            Vcb->PartitionInformation.PartitionLength.QuadPart  ) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            _SEH2_LEAVE;
        }

        if (!Nocache) {

            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart ){
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);
                Length &= ~((ULONG)SECTOR_SIZE - 1);
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcMdlRead(
                    Vcb->StreamObj,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;

            } else {

                Buffer = RfsdGetUserBuffer(Irp);
                    
                if (Buffer == NULL) {
                    DbgBreak();
                    Status = STATUS_INVALID_USER_BUFFER;
                    _SEH2_LEAVE;
                }

                if (!CcCopyRead(
                    Vcb->StreamObj,
                    (PLARGE_INTEGER)&ByteOffset,
                    Length,
                    IrpContext->IsSynchronous,
                    Buffer,
                    &Irp->IoStatus )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
                
                Status = Irp->IoStatus.Status;
            }

        } else {

            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart ) {
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);

                Length &= ~((ULONG)SECTOR_SIZE - 1);
            }

            Status = RfsdLockUserBuffer(
                IrpContext->Irp,
                Length,
                IoWriteAccess );
                
            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

#if DBG
            Buffer = RfsdGetUserBuffer(Irp);
#endif
            rfsd_bdl = ExAllocatePoolWithTag(PagedPool, sizeof(RFSD_BDL), RFSD_POOL_TAG);

            if (!rfsd_bdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            rfsd_bdl->Irp = NULL;
            rfsd_bdl->Lba = ByteOffset.QuadPart;
            rfsd_bdl->Length = Length;
            rfsd_bdl->Offset = 0;

            Status = RfsdReadWriteBlocks(IrpContext,
                                Vcb,
                                rfsd_bdl,
                                Length,
                                1,
                                FALSE   );

            Irp = IrpContext->Irp;

            if (!Irp)
                _SEH2_LEAVE;
        }

    } _SEH2_FINALLY {

        if (PagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
        }

        if (rfsd_bdl)
            ExFreePool(rfsd_bdl);

        if (!IrpContext->ExceptionInProgress) {

            if (IrpContext->Irp) {

                if (Status == STATUS_PENDING &&
                    !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED)) {

                    Status = RfsdLockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                    
                    if (NT_SUCCESS(Status)) {
                        Status = RfsdQueueRequest(IrpContext);
                    } else {
                        RfsdCompleteIrpContext(IrpContext, Status);
                    }

                } else {

                    if (NT_SUCCESS(Status)) {

                        if (!PagingIo) {

                            if (SynchronousIo) {

                                IrpContext->FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
                            }

                            IrpContext->FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    RfsdCompleteIrpContext(IrpContext, Status);;
                }

            } else {
                RfsdFreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;

    return Status;
}

// [mark] read some goop [from the file pt'd to by inode -- from whatever blocks buildbdl makes] into the buffer
NTSTATUS
RfsdReadInode (
            IN PRFSD_IRP_CONTEXT    IrpContext,				// [may be null]
            IN PRFSD_VCB            Vcb,
            IN PRFSD_KEY_IN_MEMORY  Key,					// Key that identifies the data on disk to be read.  This is simply forwarded through to BuildBDL. (NOTE: IN THIS CASE, THE OFFSET AND TYPE FIELDS MATTER)
            IN PRFSD_INODE          Inode,					// a filled Inode / stat data structure
            IN ULONGLONG            Offset,					// User's requested offset to read within the file (relative to the file)
            IN OUT PVOID            Buffer,					// buffer to read out to
            IN ULONG                Size,					// size of destination buffer
            OUT PULONG              dwRet )					// some kind of size [probably bytes read?]
{
    PRFSD_BDL   Bdl     = NULL;
    ULONG       blocks, i, j;
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK IoStatus;

    ULONGLONG   FileSize;
    ULONGLONG   AllocSize;

    PAGED_CODE();

    if (dwRet) {
        *dwRet = 0;
    }

    //
    // Calculate the inode size
    //

    FileSize = (ULONGLONG) Inode->i_size;

	//KdPrint(("Rfsd: RfsdReadInode: file size = %I64u, offset = %I64u, length = %u\n", FileSize, Offset, Size));

	// TODO: temporary hack to get correct alloc size for dir tails... but i doubt 8 works in all cases :-)  [what i should really be using is the size of the direct item in the block header!]
    // AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG)Vcb->BlockSize);
	// AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG) 8);
	AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG) 1);		// temp hack to ensure that i'll read out EXACTLY the # of bytes he requested

    //
    // Check inputed parameters: Offset / Size
    //

    if (Offset >= AllocSize) {

        RfsdPrint((DBG_ERROR, "RfsdReadInode: beyond the file range.\n"));
        return STATUS_SUCCESS;
    }

    if (Offset + Size > AllocSize) {

        Size = (ULONG)(AllocSize - Offset);
    }


//-----------------------------	  
	
    //
    // Build the scatterred block ranges to be read
    //

    Status = RfsdBuildBDL2(
		Vcb, Key, Inode, 
		&(blocks), &(Bdl)
		);

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    if (blocks <= 0) {
        Status = STATUS_SUCCESS;
        goto errorout;
    }

	
	{
	  ULONGLONG bufferPos = 0;

      for(i = 0, j = 0; i < blocks; i++) {
		  if ( // The block is needed for the user's requested contents
			   // (The user's requested offset lies within the block, or the block's start is within the user's requested range)
			  ( (Offset >= Bdl[i].Offset) && (Offset < (Bdl[i].Offset + Bdl[i].Length)) ) ||		// The user's offset is within the block's range
			  ( (Bdl[i].Offset >= Offset) && (Bdl[i].Offset < (Offset + Size)) )					// The block's offset is within the user's range
		      )
		  {
			  ULONGLONG	offsetFromDisk	= Bdl[i].Lba;			  
			  ULONGLONG	lengthToRead	= min(Size - bufferPos, Bdl[i].Length);
			  j++;

			  //KdPrint(("Rfsd: blocks = %u, i = %u, j = %u\n", blocks, i, j));
			  //KdPrint(("Rfsd: Bdl[%u].Lba = %I64u, Bdl[%u].Offset = %I64u, Bdl[%u].Length = %u\n", i, Bdl[i].Lba, i, Bdl[i].Offset, i, Bdl[i].Length));
			  //KdPrint(("Rfsd: offsetFromDisk = %I64u, lengthToRead = %I64u\n", offsetFromDisk, lengthToRead));
			  //KdPrint(("Rfsd: Buffer = %p, bufferPos = %I64u\n", Buffer, bufferPos));

			  IoStatus.Information = 0;				

			  RfsdCopyRead(
					  Vcb->StreamObj, 
					  (PLARGE_INTEGER) (&offsetFromDisk),	// offset (relative to partition)
					  (ULONG) lengthToRead,					// length to read
					  PIN_WAIT,								// 
					  (PVOID)((PUCHAR)Buffer + bufferPos),	// buffer to read into
					  &IoStatus   );

			  Status = IoStatus.Status;
			  bufferPos += IoStatus.Information;
			  //KdPrint(("Rfsd: IoStatus.Status = %#x, IoStatus.Information = %u\n", IoStatus.Status, IoStatus.Information));
		  }
      }

	}

errorout:

    if (Bdl)				ExFreePool(Bdl);

    if (NT_SUCCESS(Status)) {

        if (dwRet) *dwRet = Size;
    }

    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdReadFile(IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB           Vcb;
    PRFSD_FCB           Fcb = 0;
    PRFSD_CCB           Ccb;
    PFILE_OBJECT        FileObject;
    PFILE_OBJECT        CacheObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    ULONG               ReturnedLength;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        ASSERT(Fcb);
    
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));		

        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
        
#if defined(_MSC_VER) && !defined(__clang__)
		KdPrint(("$$$ " __FUNCTION__ " on key: %x,%xh to read %i bytes at the offset %xh in the file\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid,
			Length, ByteOffset.QuadPart));
#endif

        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

/*
        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
            Status = STATUS_DELETE_PENDING;
            _SEH2_LEAVE;
        }
*/

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (Nocache &&
           (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
            Length & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
            DbgBreak();
            _SEH2_LEAVE;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            DbgBreak();
            _SEH2_LEAVE;
        }
        
        if (!PagingIo) {
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            MainResourceAcquired = TRUE;

            if (!FsRtlCheckLockForReadAccess(
                &Fcb->FileLockAnchor,
                Irp         )) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                _SEH2_LEAVE;
            }
        } else {
            if (!ExAcquireResourceSharedLite(
                &Fcb->PagingIoResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            PagingIoResourceAcquired = TRUE;
        }
        
		if (!Nocache) {
			// Attempt cached access...

            if ((ByteOffset.QuadPart + (LONGLONG)Length) >
                Fcb->Header.FileSize.QuadPart ) {
                if (ByteOffset.QuadPart >= (Fcb->Header.FileSize.QuadPart)) {
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_END_OF_FILE;
                    _SEH2_LEAVE;
                }

                Length =
                    (ULONG)(Fcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);

            }

            ReturnedLength = Length;

            if (IsDirectory(Fcb)) {
                _SEH2_LEAVE;
            }

            {
                if (FileObject->PrivateCacheMap == NULL) {
                    CcInitializeCacheMap(
                        FileObject,
                        (PCC_FILE_SIZES)(&Fcb->Header.AllocationSize),
                        FALSE,
                        &RfsdGlobal->CacheManagerCallbacks,
                        Fcb );
                }

                CacheObject = FileObject;
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {
                CcMdlRead(
                    CacheObject,
                    (&ByteOffset),
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;

            } else {
                Buffer = RfsdGetUserBuffer(Irp);
                
                if (Buffer == NULL) {
                    Status = STATUS_INVALID_USER_BUFFER;
                    DbgBreak();
                    _SEH2_LEAVE;
                }
                
                if (!CcCopyRead(
                    CacheObject,						// the file object (representing the open operation performed by the thread)
                    (PLARGE_INTEGER)&ByteOffset,		// starting offset IN THE FILE, from where the read should be performed
                    Length,								// number of bytes requested in the read operation
                    IrpContext->IsSynchronous,
                    Buffer,								// < buffer to read the contents to
                    &Irp->IoStatus )) {
                    Status = STATUS_PENDING;
                    DbgBreak();
                    _SEH2_LEAVE;
                }
                
                Status = Irp->IoStatus.Status;
            }

        } else {
			// Attempt access without the cache...

            if ((ByteOffset.QuadPart + (LONGLONG)Length) > Fcb->Header.AllocationSize.QuadPart) {

                if (ByteOffset.QuadPart >= Fcb->Header.AllocationSize.QuadPart) {
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_END_OF_FILE;
                    DbgBreak();
                    _SEH2_LEAVE;
                }

                Length =
                     (ULONG)(Fcb->Header.AllocationSize.QuadPart- ByteOffset.QuadPart);
            }

            ReturnedLength = Length;

            /* lock the user buffer into MDL and make them paged-in */
            Status = RfsdLockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                
            if (NT_SUCCESS(Status)) {

                /* Zero the total buffer */
                PVOID SystemVA = RfsdGetUserBuffer(IrpContext->Irp);
                if (SystemVA) {

                    RtlZeroMemory(SystemVA, Length);

                    RfsdPrint((DBG_INFO, "RfsdReadFile: Zero read buffer: Offset=%I64xh Size=%xh ... \n",
                               ByteOffset.QuadPart, Length));
                }

            } else {
                _SEH2_LEAVE;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Length;

            
            Status = RfsdReadInode(
                        IrpContext,
                        Vcb,
                        &(Fcb->RfsdMcb->Key),
                        Fcb->Inode,
                        ByteOffset.QuadPart,
                        RfsdGetUserBuffer(IrpContext->Irp),		//  NOTE: Ext2fsd just passes NULL for the buffer, and relies on the initial cache call to retrieve tha data.  We'll instead be explicitly putting it into the user's buffer, via a much different mechanism.
                        Length,
                        &ReturnedLength);

            Irp = IrpContext->Irp;

        }

    } _SEH2_FINALLY {

        if (PagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (IrpContext->Irp) {
                if (Status == STATUS_PENDING) {

                    Status = RfsdLockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                    
                    if (NT_SUCCESS(Status)) {
                        Status = RfsdQueueRequest(IrpContext);
                    } else {
                        RfsdCompleteIrpContext(IrpContext, Status);
                    }
                } else {
                    if (NT_SUCCESS(Status)) {
                        if (!PagingIo) {
                            if (SynchronousIo) {
                                IrpContext->FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
                            }

                            IrpContext->FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    RfsdCompleteIrpContext(IrpContext, Status);
                }
            } else {
                RfsdFreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;
    
    return Status;
}

NTSTATUS
RfsdReadComplete (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        FileObject = IrpContext->FileObject;
        
        Irp = IrpContext->Irp;
        
        CcMdlReadComplete(FileObject, Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
    
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdRead (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PRFSD_VCB           Vcb;
    PRFSD_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    BOOLEAN             bCompleteRequest;

    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

    _SEH2_TRY {

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {
			// Caller wants to tell the Cache Manager that a previously allocated MDL can be freed.
            Status =  RfsdReadComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;
                _SEH2_LEAVE;
            }

            Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;

            if (Vcb->Identifier.Type != RFSDVCB ||
                Vcb->Identifier.Size != sizeof(RFSD_VCB) ) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;

                _SEH2_LEAVE;
            }

            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {

                Status = STATUS_TOO_LATE;
                bCompleteRequest = TRUE;
                _SEH2_LEAVE;
            }

            FileObject = IrpContext->FileObject;
            
            FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == RFSDVCB) {
                Status = RfsdReadVolume(IrpContext);
                bCompleteRequest = FALSE;
            } else if (FcbOrVcb->Identifier.Type == RFSDFCB) {
                Status = RfsdReadFile(IrpContext);
                bCompleteRequest = FALSE;
            } else {
                RfsdPrint((DBG_ERROR, "RfsdRead: INVALID PARAMETER ... \n"));
                DbgBreak();

                Status = STATUS_INVALID_PARAMETER;
                bCompleteRequest = TRUE;
            }
        }

    } _SEH2_FINALLY {
        if (bCompleteRequest) {
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
    
    return Status;
}

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/cachesup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Art Yerkes
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

BOOLEAN
NTAPI
MmIsCOWAddress(PEPROCESS Process, PVOID Address);

NTSTATUS
NTAPI
CcpSimpleWrite
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 PVOID Buffer,
 PIO_STATUS_BLOCK ReadStatus)
{
    ULONG Length = PAGE_SIZE;
    NTSTATUS Status;
    PIRP Irp = NULL;
    KEVENT ReadWait;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IrpSp;
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    if (MmIsDirtyPage(PsInitialSystemProcess, Buffer) && 
	!MmIsCOWAddress(PsInitialSystemProcess, Buffer))
    {
	DPRINT1
	    ("PAGING WRITE (FLUSH) Offset %x Length %d\n", 
	     FileOffset->u.LowPart,
	     Length);

	DeviceObject = MmGetDeviceObjectForFile(FileObject);
	MmSetCleanPage(PsInitialSystemProcess, Buffer);
	
	KeLowerIrql(OldIrql);

	KeInitializeEvent(&ReadWait, NotificationEvent, FALSE);
	
	Irp = IoBuildAsynchronousFsdRequest
	    (IRP_MJ_WRITE,
	     DeviceObject,
	     Buffer,
	     Length,
	     FileOffset,
	     ReadStatus);
	
	if (!Irp)
	{
	    return STATUS_NO_MEMORY;
	}
	
	Irp->Flags |= IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE;
	
	ObReferenceObject(FileObject);
	
	Irp->UserEvent = &ReadWait;
	Irp->Tail.Overlay.OriginalFileObject = FileObject;
	Irp->Tail.Overlay.Thread = PsGetCurrentThread();
	IrpSp = IoGetNextIrpStackLocation(Irp);
	IrpSp->FileObject = FileObject;
	
	Status = IoCallDriver(DeviceObject, Irp);
	if (Status == STATUS_PENDING)
	{
	    if (!NT_SUCCESS
		(KeWaitForSingleObject
		 (&ReadWait, 
		  Suspended, 
		  KernelMode, 
		  FALSE, 
		  NULL)))
	    {
		DPRINT1("Warning: Failed to wait for synchronous IRP\n");
		ASSERT(FALSE);
		ObDereferenceObject(FileObject);
		return Status;
	    }
	}
	
	ObDereferenceObject(FileObject);
	MmDeleteHyperspaceMapping(Buffer);

	DPRINT("Paging IO Done: %08x\n", ReadStatus->Status);
    }
    else
    {
	ReadStatus->Status = STATUS_SUCCESS;
	ReadStatus->Information = PAGE_SIZE;
	KeLowerIrql(OldIrql);
    }
    
    return ReadStatus->Status;
}

VOID
NTAPI
CcSetReadAheadGranularity(IN PFILE_OBJECT FileObject,
                          IN ULONG Granularity)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcScheduleReadAhead(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN ULONG Length)
{
    UNIMPLEMENTED;
    while (TRUE);  
}

VOID
NTAPI
CcSetDirtyPinnedData(IN PVOID BcbVoid,
                     IN OPTIONAL PLARGE_INTEGER Lsn)
{
	PCHAR Buffer;
	PNOCC_BCB Bcb = (PNOCC_BCB)BcbVoid;
	Bcb->Dirty = TRUE;
	for (Buffer = Bcb->BaseAddress; 
		 Buffer < ((PCHAR)Bcb->BaseAddress) + Bcb->Length;
		 Buffer += PAGE_SIZE)
	{
		/* Do a fake write on the buffer pages to mark them for mm */
		*Buffer ^= 0;
	}
}

LARGE_INTEGER
NTAPI
CcGetFlushedValidData(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN BOOLEAN CcInternalCaller)
{
    LARGE_INTEGER Result = {{0}};
    UNIMPLEMENTED;
    while (TRUE);
    return Result;
}

BOOLEAN
NTAPI
CcpMapData
(IN PNOCC_CACHE_MAP Map,
 IN PLARGE_INTEGER FileOffset,
 IN ULONG Length,
 IN ULONG Flags,
 OUT PVOID *BcbResult,
 OUT PVOID *Buffer);


VOID
NTAPI
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
             IN OPTIONAL PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             OUT OPTIONAL PIO_STATUS_BLOCK IoStatus)
{
	PCHAR BufPage, BufStart;
	PVOID Buffer;
	PNOCC_BCB Bcb;
	LARGE_INTEGER ToWrite = *FileOffset;
	IO_STATUS_BLOCK IOSB;

	BOOLEAN Result = CcpMapData
		((PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap,
		 FileOffset,
		 Length,
		 PIN_WAIT,
		 (PVOID *)&Bcb,
		 &Buffer);

	if (!Result) return;

	BufStart = (PCHAR)PAGE_ROUND_DOWN(((ULONG_PTR)Buffer));
	ToWrite.LowPart = PAGE_ROUND_DOWN(FileOffset->LowPart);

	DPRINT
	    ("CcpSimpleWrite: [%wZ] %x:%d\n", 
	     &Bcb->FileObject->FileName,
	     Buffer,
	     Bcb->Length);

	for (BufPage = BufStart;
	     BufPage < BufStart + PAGE_ROUND_UP(Length);
	     BufPage += PAGE_SIZE)
	{
	    CcpSimpleWrite(Bcb->FileObject, &ToWrite, BufPage, &IOSB);
	    ToWrite.QuadPart += PAGE_SIZE;
	}

	DPRINT
	    ("Page Write: %08x\n", IOSB.Status);

	CcUnpinData(Bcb);

	if (IoStatus && NT_SUCCESS(IOSB.Status))
	{
		IoStatus->Status = STATUS_SUCCESS;
		IoStatus->Information = Length;
	}
	else if (IoStatus)
	{
		IoStatus->Status = IOSB.Status;
		IoStatus->Information = 0;
	}
}

// Always succeeds for us
PVOID
NTAPI
CcRemapBcb(IN PVOID Bcb)
{
    return Bcb;
}


VOID
NTAPI
CcRepinBcb(IN PVOID Bcb)
{
    PVOID TheBcb;
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    CcPinMappedData
	(RealBcb->FileObject, 
	 &RealBcb->FileOffset,
	 RealBcb->Length,
	 PIN_WAIT,
	 &TheBcb);
}

VOID
NTAPI
CcUnpinRepinnedBcb(IN PVOID Bcb,
                   IN BOOLEAN WriteThrough,
                   OUT PIO_STATUS_BLOCK IoStatus)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)RealBcb;

    CcUnpinData(Bcb);

    if (WriteThrough)
    {
	CcFlushCache
	    (RealBcb->FileObject->SectionObjectPointer,
	     &RealBcb->FileOffset,
	     RealBcb->Length,
	     IoStatus);
    }
}

/* EOF */

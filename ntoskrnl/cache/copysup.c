/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/copysup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG CcFastMdlReadWait;
ULONG CcFastMdlReadNotPossible;
ULONG CcFastReadNotPossible;
ULONG CcFastReadWait;
ULONG CcFastReadNoWait;
ULONG CcFastReadResourceMiss;

#define TAG_COPY_READ  TAG('C', 'o', 'p', 'y')
#define TAG_COPY_WRITE TAG('R', 'i', 't', 'e')

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CcCopyRead(IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER FileOffset,
           IN ULONG Length,
           IN BOOLEAN Wait,
           OUT PVOID Buffer,
           OUT PIO_STATUS_BLOCK IoStatus)
{
    PCHAR ReadBuffer;
    ULONG ReadLen;
    PVOID Bcb;
    PCHAR BufferTarget = (PCHAR)Buffer;
    LARGE_INTEGER CacheOffset, EndOfExtent, NextOffset;
    
    DPRINT
	("CcCopyRead(%x,%x,%d,%d,%x)\n", 
	 FileObject, 
	 FileOffset->LowPart,
	 Length,
	 Wait,
	 Buffer);
    
    CacheOffset.QuadPart = FileOffset->QuadPart;
    EndOfExtent.QuadPart = FileOffset->QuadPart + Length;

    while (CacheOffset.QuadPart < EndOfExtent.QuadPart)
    {
		NextOffset.QuadPart = (CacheOffset.QuadPart + CACHE_STRIPE) & ~(CACHE_STRIPE-1);
		ReadLen = EndOfExtent.QuadPart - CacheOffset.QuadPart;
		if (CacheOffset.QuadPart + ReadLen > NextOffset.QuadPart)
		{
			ReadLen = NextOffset.QuadPart - CacheOffset.QuadPart;
		}
		
		DPRINT("Reading %d bytes in this go (at %08x%08x)\n", ReadLen, CacheOffset.HighPart, CacheOffset.LowPart);
		
		if (!CcPinRead
			(FileObject,
			 &CacheOffset,
			 ReadLen,
			 Wait ? PIN_WAIT : PIN_IF_BCB,
			 &Bcb,
			 (PVOID*)&ReadBuffer))
		{
			IoStatus->Status = STATUS_UNSUCCESSFUL;
			IoStatus->Information = 0;
			DPRINT("Failed CcCopyRead\n");
			return FALSE;
		}
		
		DPRINT("Copying %d bytes at %08x%08x\n", ReadLen, CacheOffset.HighPart, CacheOffset.LowPart);
		RtlCopyMemory
			(BufferTarget,
			 ReadBuffer,
			 ReadLen);
		
		BufferTarget += ReadLen;
		
		CacheOffset = NextOffset;
		CcUnpinData(Bcb);
    }
	
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = Length;
    
    DPRINT("Done with CcCopyRead\n");
    
    return TRUE;
}

VOID
NTAPI
CcFastCopyRead(IN PFILE_OBJECT FileObject,
               IN ULONG FileOffset,
               IN ULONG Length,
               IN ULONG PageCount,
               OUT PVOID Buffer,
               OUT PIO_STATUS_BLOCK IoStatus)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
CcCopyWrite(IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN ULONG Length,
            IN BOOLEAN Wait,
            IN PVOID Buffer)
{
    PCHAR WriteBuffer;
    ULONG WriteLen;
    PCHAR BufferTarget = (PCHAR)Buffer;
    PNOCC_BCB Bcb;
	IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER CacheOffset, EndOfExtent, NextOffset;
    
    DPRINT
	("CcCopyWrite(%x,%x,%d,%d,%x)\n", 
	 FileObject, 
	 FileOffset->LowPart,
	 Length,
	 Wait,
	 Buffer);
    
    CacheOffset.QuadPart = FileOffset->QuadPart;
    EndOfExtent.QuadPart = FileOffset->QuadPart + Length;

    while (CacheOffset.QuadPart < EndOfExtent.QuadPart)
    {
		NextOffset.QuadPart = (CacheOffset.QuadPart + CACHE_STRIPE) & ~(CACHE_STRIPE-1);
		WriteLen = EndOfExtent.QuadPart - CacheOffset.QuadPart;
		if (CacheOffset.QuadPart + WriteLen > NextOffset.QuadPart)
		{
			WriteLen = NextOffset.QuadPart - CacheOffset.QuadPart;
		}
		
		DPRINT("Writeing %d bytes in this go (at %08x%08x)\n", WriteLen, CacheOffset.HighPart, CacheOffset.LowPart);
		
		if (!CcPreparePinWrite
			(FileObject,
			 &CacheOffset,
			 WriteLen,
			 (CacheOffset.QuadPart == Bcb->FileOffset.QuadPart && 
			  WriteLen == CACHE_STRIPE),
			 Wait ? PIN_WAIT : PIN_IF_BCB,
			 (PVOID*)&Bcb,
			 (PVOID*)&WriteBuffer))
		{
			DPRINT("Failed CcCopyWrite\n");
			return FALSE;
		}
		
		DPRINT("Copying %d bytes at %08x%08x\n", WriteLen, CacheOffset.HighPart, CacheOffset.LowPart);
		RtlCopyMemory
			(WriteBuffer,
			 BufferTarget,
			 WriteLen);
		
		BufferTarget += WriteLen;
		
		CcFlushCache
			(Bcb->FileObject->SectionObjectPointer,
			 &CacheOffset,
			 WriteLen,
			 &IoStatus);
		CacheOffset = NextOffset;
		CcUnpinData(Bcb);
    }
	
    DPRINT("Done with CcCopyWrite\n");
    
    return TRUE;
}

VOID
NTAPI
CcFastCopyWrite(IN PFILE_OBJECT FileObject,
                IN ULONG FileOffset,
                IN ULONG Length,
                IN PVOID Buffer)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
CcCanIWrite(IN PFILE_OBJECT FileObject,
            IN ULONG BytesToWrite,
            IN BOOLEAN Wait,
            IN UCHAR Retrying)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
CcDeferWrite(IN PFILE_OBJECT FileObject,
             IN PCC_POST_DEFERRED_WRITE PostRoutine,
             IN PVOID Context1,
             IN PVOID Context2,
             IN ULONG BytesToWrite,
             IN BOOLEAN Retrying)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */

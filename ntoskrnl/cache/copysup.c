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
	INT i, Count;
	PCHAR ReadBuf;
	ULONG ReadLen;
	PVOID Bcbs[CACHE_NUM_SECTIONS];
	PVOID ReadBuffers[CACHE_NUM_SECTIONS];

	DPRINT
		("CcCopyRead(%x,%x,%d,%d,%x)\n", 
		 FileObject, 
		 FileOffset->LowPart,
		 Length,
		 Wait,
		 Buffer);

	for (ReadLen = Length, i = 0; 
		 ReadLen > 0;
		 ReadLen -= min(ReadLen, CACHE_STRIPE), i++)
	{
		if (!CcPinRead
			(FileObject,
			 FileOffset,
			 min(ReadLen, CACHE_STRIPE),
			 Wait ? PIN_WAIT : PIN_IF_BCB,
			 &Bcbs[i],
			 &ReadBuffers[i]))
		{
			--i;
			while (i >= 0)
				CcUnpinData(Bcbs[i--]);
			IoStatus->Status = STATUS_UNSUCCESSFUL;
			IoStatus->Information = 0;
			DPRINT("Failed CcCopyRead\n");
			return FALSE;
		}
	}

	Count = i;

	DPRINT("Copying %d bytes for Read (%d buffers)\n", Length, Count);
	for (i = 0; i < Count; i++)
	{
		DPRINT("  %d: [#%02x:%x]\n", i, Bcbs[i], ReadBuffers[i]);
	}

	for (ReadBuf = (PCHAR)Buffer, ReadLen = Length, i = 0;
		 ReadLen > 0;
		 ReadBuf += CACHE_STRIPE, ReadLen -= min(ReadLen, CACHE_STRIPE), i++)
		RtlCopyMemory(ReadBuf, ReadBuffers[i], min(ReadLen, CACHE_STRIPE));

	for (i = 0; i < Count; i++)
		CcUnpinData(Bcbs[i]);

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
	INT i, Count;
	PCHAR WriteBuf;
	ULONG WriteLen;
	PVOID Bcbs[CACHE_NUM_SECTIONS];
	PVOID WriteBuffers[CACHE_NUM_SECTIONS];

	DPRINT
		("CcCopyWrite(%x,%x,%d,%d,%x)\n", 
		 FileObject, 
		 FileOffset->LowPart,
		 Length,
		 Wait,
		 Buffer);

	for (WriteLen = Length, i = 0; 
		 WriteLen > 0;
		 WriteLen -= min(WriteLen, CACHE_STRIPE), i++)
	{
		if (!CcPreparePinWrite
			(FileObject,
			 FileOffset,
			 min(WriteLen, CACHE_STRIPE),
			 FALSE,
			 Wait ? PIN_WAIT : PIN_IF_BCB,
			 &Bcbs[i],
			 &WriteBuffers[i]))
		{
			--i;
			while (i >= 0)
				CcUnpinData(Bcbs[i--]);
			DPRINT("Failed CcCopyWrite\n");
			return FALSE;
		}
	}

	Count = i;

	DPRINT("Copying %d bytes for Read\n", Length);

	for (WriteBuf = (PCHAR)Buffer, WriteLen = Length, i = 0;
		 WriteLen > 0;
		 WriteBuf += CACHE_STRIPE, WriteLen -= min(WriteLen, CACHE_STRIPE), i++)
		RtlCopyMemory(WriteBuffers[i], WriteBuf, min(WriteLen, CACHE_STRIPE));

	Count = i;

	DPRINT("Copying %d bytes for Write\n", Length);
	for (i = 0; i < Count; i++)
	{
		DPRINT("  %d: [#%02x:%x]\n", i, Bcbs[i], WriteBuffers[i]);
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

/* $Id: 
 *
 *
 * FILE:             drivers/fs/vfat/fastio.c
 * PURPOSE:          Fast IO routines.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Herve Poussineau (reactos@poussine.freesurf.fr)
 */

#define NDEBUG
#include "vfat.h"

BOOLEAN NTAPI
VfatFastIoCheckIfPossible(IN PFILE_OBJECT FileObject,
                          IN PLARGE_INTEGER FileOffset,
                          IN ULONG Lenght,
                          IN BOOLEAN Wait,
                          IN ULONG LockKey,
                          IN BOOLEAN CheckForReadOperation,
                          OUT PIO_STATUS_BLOCK IoStatus,
                          IN PDEVICE_OBJECT DeviceObject)
{
	/* Prevent all Fast I/O requests */
	DPRINT("VfatFastIoCheckIfPossible(): returning FALSE.\n");
	return FALSE;
}

BOOLEAN NTAPI
VfatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait)
{
   PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatAcquireForLazyWrite(): Fcb %p\n", Fcb);
	
	if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
	{
		DPRINT("VfatAcquireForLazyWrite(): ExReleaseResourceLite failed.\n");
		return FALSE;
	}
	return TRUE;
}

VOID NTAPI
VfatReleaseFromLazyWrite(IN PVOID Context)
{
   PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatReleaseFromLazyWrite(): Fcb %p\n", Fcb);
	
	ExReleaseResourceLite(&(Fcb->MainResource));
}

BOOLEAN NTAPI
VfatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait)
{
   PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatAcquireForReadAhead(): Fcb %p\n", Fcb);
	
	if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
	{
		DPRINT("VfatAcquireForReadAhead(): ExReleaseResourceLite failed.\n");
		return FALSE;
	}
	return TRUE;
}                        	

VOID NTAPI
VfatReleaseFromReadAhead(IN PVOID Context)
{
   PVFATFCB Fcb = (PVFATFCB)Context;
	ASSERT(Fcb);
	DPRINT("VfatReleaseFromReadAhead(): Fcb %p\n", Fcb);
	
	ExReleaseResourceLite(&(Fcb->MainResource));
}

/* $Id: device.c,v 1.3 2004/10/13 01:42:14 ion Exp $
 *
 * FILE:            ntoskrnl/ke/profile.c
 * PURPOSE:         Kernel Device/Settings Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 23/06/04
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/*
 * @unimplemented
 */
STDCALL
PVOID
KeFindConfigurationEntry(
    IN PVOID Unknown,
    IN ULONG Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG RegKey
)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
STDCALL
PVOID
KeFindConfigurationNextEntry(
    IN PVOID Unknown,
    IN ULONG Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG RegKey,
    IN PVOID *NextLink
)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */
STDCALL
VOID
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
)
{
	KIRQL OldIrql;
	PKPROCESS Process;
	PKPCR Pcr;
	
	/* Raise the IRQL for the TB Flush */
	OldIrql = KeRaiseIrqlToSynchLevel();
	
	/* All CPUs need to have the TB flushed. */
	if (CurrentCpuOnly == FALSE) {
		Pcr = KeGetCurrentKPCR();
		
		/* How many CPUs is our caller using? */
		Process = Pcr->PrcbData.CurrentThread->ApcState.Process;
		
		/* More then one, so send an IPI */
		if (Process->ActiveProcessors > 1) {
			/* Send IPI Packet */
		}
	}
	
	/* Flush the TB for the Current CPU */
	KeFlushCurrentTb();
	
	/* Clean up */
	if (CurrentCpuOnly == FALSE) {
		/* Did we send an IPI? If so, wait for completion */
		if (Process->ActiveProcessors > 1) {
			do {
			} while (Pcr->PrcbData.TargetSet != 0);
		} 
	} 
	
	/* FIXME: According to MSKB, we should increment a counter? */
	
	/* Return to Original IRQL */	
	KeLowerIrql(OldIrql);
}


/*
 * @unimplemented
 */
STDCALL
VOID
KeSetDmaIoCoherency(
    IN ULONG Coherency
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueueIfBusy (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    )
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
STDCALL
KAFFINITY
KeQueryActiveProcessors (
    VOID
    )
{
	UNIMPLEMENTED;
	return 0;
}


/*
 * @unimplemented
 */
VOID
__cdecl
KeSaveStateForHibernate(
    IN PVOID State
)
{
	UNIMPLEMENTED;
}

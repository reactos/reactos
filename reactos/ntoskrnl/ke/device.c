/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/device.c
 * PURPOSE:         Kernel Device/Settings Functions
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
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
	/* Start Search at Root */
	return KeFindConfigurationNextEntry(Unknown, Class, Type, RegKey, NULL);
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
	PKPROCESS Process = NULL;
	PKPRCB Prcb = NULL;
	
	/* Raise the IRQL for the TB Flush */
	OldIrql = KeRaiseIrqlToSynchLevel();
	
	/* All CPUs need to have the TB flushed. */
	if (CurrentCpuOnly == FALSE) {
	        Prcb = KeGetCurrentPrcb();

		/* How many CPUs is our caller using? */
		Process = Prcb->CurrentThread->ApcState.Process;
		
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
			} while (Prcb->TargetSet != 0);
		} 
	} 
	
	/* FIXME: According to MSKB, we should increment a counter? */
	
	/* Return to Original IRQL */	
	KeLowerIrql(OldIrql);
}


/*
 * @implemented
 */
STDCALL
VOID
KeSetDmaIoCoherency(
    IN ULONG Coherency
)
{
	KiDmaIoCoherency = Coherency;
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
 * @implemented
 */
STDCALL
KAFFINITY
KeQueryActiveProcessors (
    VOID
    )
{
	return KeActiveProcessors;
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

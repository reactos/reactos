/* $Id: device.c,v 1.2 2004/08/15 16:39:05 chorns Exp $
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
 * @unimplemented
 */
STDCALL
VOID
KeFlushEntireTb(
    IN ULONGLONG Flag
)
{
	UNIMPLEMENTED;
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

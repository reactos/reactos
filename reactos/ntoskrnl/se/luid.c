/* $Id: luid.c,v 1.2 1999/12/26 17:22:19 ea Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              ntoskrnl/se/luid.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK LuidLock;
static LARGE_INTEGER LuidIncrement;
static LUID Luid;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtAllocateLocallyUniqueId(OUT LUID* LocallyUniqueId)
{
   KIRQL oldIrql;
   LUID ReturnedLuid;
   
   KeAcquireSpinLock(&LuidLock, &oldIrql);
   ReturnedLuid = Luid;
   Luid = RtlLargeIntegerAdd(Luid, LuidIncrement);
   KeReleaseSpinLock(&LuidLock, oldIrql);
   *LocallyUniqueId = ReturnedLuid;
   return(STATUS_SUCCESS);
}

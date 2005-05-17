/* $Id: kd.h 13948 2005-03-12 01:11:06Z navaraf $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KD_BOCHS_H
#define __INCLUDE_INTERNAL_KD_BOCHS_H

#include <internal/ke.h>
#include <internal/ldr.h>
#include <ntdll/ldr.h>

VOID
STDCALL
KdpBochsInit(struct _KD_DISPATCH_TABLE *DispatchTable,
             ULONG BootPhase);

#endif /* __INCLUDE_INTERNAL_KD_BOCHS_H */

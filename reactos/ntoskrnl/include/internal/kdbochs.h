/* $Id: kd.h 13948 2005-03-12 01:11:06Z navaraf $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KD_BOCHS_H
#define __INCLUDE_INTERNAL_KD_BOCHS_H

VOID
STDCALL
KdpBochsInit(struct _KD_DISPATCH_TABLE *DispatchTable,
             ULONG BootPhase);
VOID
STDCALL
KdpBochsDebugPrint(IN PCH Message);

#endif /* __INCLUDE_INTERNAL_KD_BOCHS_H */

/* $Id: kd.h,v 1.3 2001/02/10 22:51:08 dwelch Exp $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

ULONG 
KdpPrintString (PANSI_STRING String);

VOID 
DebugLogWrite(PCH String);
VOID
DebugLogInit(VOID);
VOID
DebugLogInit2(VOID);

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */

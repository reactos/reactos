/* $Id: kd.h,v 1.4 2002/01/23 23:39:25 chorns Exp $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

#include <internal/ke.h>

typedef enum _KD_CONTINUE_TYPE
{
  kdContinue = 0,
  kdDoNotHandleException,
  kdHandleException
} KD_CONTINUE_TYPE;

ULONG 
KdpPrintString (PANSI_STRING String);

VOID 
DebugLogWrite(PCH String);
VOID
DebugLogInit(VOID);
VOID
DebugLogInit2(VOID);

VOID
KdInit1();

VOID
KdInit2();

VOID
KdPutChar(UCHAR Value);

UCHAR
KdGetChar();

VOID
KdGdbStubInit();

VOID
KdDebugPrint (LPSTR Message);

KD_CONTINUE_TYPE
KdEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
  PCONTEXT Context,
	PKTRAP_FRAME TrapFrame);

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */

/* $Id: kd.h,v 1.9 2002/05/16 06:39:30 ekohl Exp $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

#include <internal/ke.h>

#define KD_DEBUG_DISABLED	0x00
#define KD_DEBUG_GDB		0x01
#define KD_DEBUG_PICE		0x02
#define KD_DEBUG_SCREEN		0x04
#define KD_DEBUG_SERIAL		0x08
#define KD_DEBUG_BOCHS		0x10
#define KD_DEBUG_FILELOG	0x20

extern ULONG KdDebugState;

KD_PORT_INFORMATION GdbPortInfo;
KD_PORT_INFORMATION LogPortInfo;

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
KdInit1(VOID);

VOID
KdInit2(VOID);

VOID
KdPutChar(UCHAR Value);

UCHAR
KdGetChar(VOID);

VOID
KdGdbStubInit();

VOID
KdGdbDebugPrint (LPSTR Message);

VOID
KdDebugPrint (LPSTR Message);

KD_CONTINUE_TYPE
KdEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
  PCONTEXT Context,
	PKTRAP_FRAME TrapFrame);

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */

/* $Id: kd.h,v 1.13 2002/07/19 03:56:47 ei Exp $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

#include <internal/ke.h>
#include <internal/ldr.h>

#define KD_DEBUG_DISABLED	0x00
#define KD_DEBUG_GDB		0x01
#define KD_DEBUG_PICE		0x02
#define KD_DEBUG_SCREEN		0x04
#define KD_DEBUG_SERIAL		0x08
#define KD_DEBUG_BOCHS		0x10
#define KD_DEBUG_FILELOG	0x20
#define KD_DEBUG_MDA            0x40

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
VOID KdInitializeMda(VOID);
VOID KdPrintMda(PCH pch);

#ifndef KDBG
#define KDB_DELETEPROCESS_HOOK(PROCESS)
#define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)
#define KDB_UNLOADDRIVER_HOOK(MODULE)
#define KDB_LOADERINIT_HOOK(NTOS, HAL)
#define KDB_SYMBOLFILE_HOOK(LOADBASE, FILENAME, LENGTH)
#else
#define KDB_DELETEPROCESS_HOOK(PROCESS) KdbFreeSymbolsProcess(PROCESS)
#define KDB_LOADDRIVER_HOOK(FILENAME, MODULE) KdbLoadDriver(FILENAME, MODULE)
#define KDB_UNLOADDRIVER_HOOK(MODULE) KdbUnloadDriver(MODULE)
#define KDB_LOADERINIT_HOOK(NTOS, HAL) KdbLdrInit(NTOS, HAL)
#define KDB_SYMBOLFILE_HOOK(LOADBASE, FILENAME, LENGTH) \
        KdbProcessSymbolFile(LOADBASE, FILENAME, LENGTH)
#endif /* KDBG */

VOID
KdbLdrLoadUserModuleSymbols(PLDR_MODULE LdrModule);
VOID
KdbProcessSymbolFile(PVOID ModuleLoadBase, PCHAR FileName, ULONG Length);
VOID
KdbLdrInit(MODULE_TEXT_SECTION* NtoskrnlTextSection,
	   MODULE_TEXT_SECTION* LdrHalTextSection);
VOID
KdbUnloadDriver(PMODULE_OBJECT ModuleObject);
VOID
KdbLoadDriver(PUNICODE_STRING Filename, PMODULE_OBJECT Module);
VOID
KdbFreeSymbolsProcess(PPEB Peb);
BOOLEAN
KdbPrintAddress(PVOID address);

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */

/* $Id$
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

#include <internal/ke.h>
#include <internal/ldr.h>
#include <ntdll/ldr.h>

#define KD_DEBUG_DISABLED	0x00
#define KD_DEBUG_GDB		0x01
#define KD_DEBUG_PICE		0x02
#define KD_DEBUG_SCREEN		0x04
#define KD_DEBUG_SERIAL		0x08
#define KD_DEBUG_BOCHS		0x10
#define KD_DEBUG_BOOTLOG	0x20
#define KD_DEBUG_MDA            0x40
#define KD_DEBUG_KDB            0x80
#define KD_DEBUG_KDSERIAL       0x100
#define KD_DEBUG_KDNOECHO       0x200

extern ULONG KdDebugState;

KD_PORT_INFORMATION GdbPortInfo;
KD_PORT_INFORMATION LogPortInfo;

typedef enum _KD_CONTINUE_TYPE
{
  kdContinue = 0,
  kdDoNotHandleException,
  kdHandleException
} KD_CONTINUE_TYPE;

VOID
KbdDisableMouse();

VOID
KbdEnableMouse();

ULONG
KdpPrintString (PANSI_STRING String);

VOID
DebugLogWrite(PCH String);
VOID
DebugLogInit(VOID);
VOID
DebugLogInit2(VOID);

VOID
STDCALL
KdDisableDebugger(
    VOID
    );

VOID
STDCALL
KdEnableDebugger(
    VOID
    );

NTSTATUS
STDCALL
KdPowerTransition(
	ULONG PowerState
	);

BOOLEAN
STDCALL
KeIsAttachedProcess(
	VOID
	);

VOID
KdInit1(VOID);

VOID
KdInit2(VOID);

VOID
KdInit3(VOID);

VOID
KdPutChar(UCHAR Value);

UCHAR
KdGetChar(VOID);

VOID
KdGdbStubInit(ULONG Phase);

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

#if !defined(KDBG) && !defined(DBG)
# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	do { } while (0)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	do { } while (0)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		do { } while (0)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		do { } while (0)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		do { } while (0)
# define KDB_CREATE_THREAD_HOOK(CONTEXT)	do { } while (0)
#else
# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	KdbSymLoadUserModuleSymbols(LDRMOD)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	KdbSymLoadDriverSymbols(FILENAME, MODULE)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		KdbSymUnloadDriverSymbols(MODULE)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		KdbSymInit(NTOS, HAL)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		KdbSymProcessBootSymbols(FILENAME)
/*#define KDB_CREATE_THREAD_HOOK(CONTEXT) \
        KdbCreateThreadHook(CONTEXT)
*/

VOID
KdbSymLoadUserModuleSymbols(IN PLDR_MODULE LdrModule);

VOID
KdbSymFreeProcessSymbols(IN PEPROCESS Process);

VOID
KdbSymLoadDriverSymbols(IN PUNICODE_STRING Filename,
                        IN PMODULE_OBJECT Module);

VOID
KdbSymUnloadDriverSymbols(IN PMODULE_OBJECT ModuleObject);

VOID
KdbSymProcessBootSymbols(IN PCHAR FileName);

VOID
KdbSymInit(IN PMODULE_TEXT_SECTION NtoskrnlTextSection,
           IN PMODULE_TEXT_SECTION LdrHalTextSection);


BOOLEAN 
KdbSymPrintAddress(IN PVOID Address);

KD_CONTINUE_TYPE
KdbEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
			  KPROCESSOR_MODE PreviousMode,
                          PCONTEXT Context,
                          PKTRAP_FRAME TrapFrame,
			  BOOLEAN FirstChance);

#endif /* KDBG || DBG */

#if !defined(KDBG)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	do { } while (0)
#else
# define KDB_DELETEPROCESS_HOOK(PROCESS)	KdbDeleteProcessHook(PROCESS)
VOID
KdbDeleteProcessHook(IN PEPROCESS Process);
#endif /* KDBG */

VOID
DebugLogDumpMessages(VOID);

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */

/******************************************************************************
 *                          Kernel Debugger Functions                         *
 ******************************************************************************/
$if (_NTDDK_)
NTSYSAPI
ULONG
NTAPI
DbgPrompt(
  IN PCCH Prompt,
  OUT PCH Response,
  IN ULONG MaximumResponseLength);
$endif

$if (_WDMDDK_)
#ifndef _DBGNT_

ULONG
__cdecl
DbgPrint(
  IN PCSTR Format,
  IN ...);

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONG
__cdecl
DbgPrintReturnControlC(
  IN PCCH Format,
  IN ...);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
  IN ULONG ComponentId,
  IN ULONG Level,
  IN PCSTR Format,
  IN ...);

#ifdef _VA_LIST_DEFINED

NTSYSAPI
ULONG
NTAPI
vDbgPrintEx(
  IN ULONG ComponentId,
  IN ULONG Level,
  IN PCCH Format,
  IN va_list ap);

NTSYSAPI
ULONG
NTAPI
vDbgPrintExWithPrefix(
  IN PCCH Prefix,
  IN ULONG ComponentId,
  IN ULONG Level,
  IN PCCH Format,
  IN va_list ap);

#endif /* _VA_LIST_DEFINED */

NTSYSAPI
NTSTATUS
NTAPI
DbgQueryDebugFilterState(
  IN ULONG ComponentId,
  IN ULONG Level);

NTSYSAPI
NTSTATUS
NTAPI
DbgSetDebugFilterState(
  IN ULONG ComponentId,
  IN ULONG Level,
  IN BOOLEAN State);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef VOID
(*PDEBUG_PRINT_CALLBACK)(
  IN PSTRING Output,
  IN ULONG ComponentId,
  IN ULONG Level);

NTSYSAPI
NTSTATUS
NTAPI
DbgSetDebugPrintCallback(
  IN PDEBUG_PRINT_CALLBACK DebugPrintCallback,
  IN BOOLEAN Enable);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#endif /* _DBGNT_ */

#if DBG

#define KdPrint(_x_) DbgPrint _x_
#define KdPrintEx(_x_) DbgPrintEx _x_
#define vKdPrintEx(_x_) vDbgPrintEx _x_
#define vKdPrintExWithPrefix(_x_) vDbgPrintExWithPrefix _x_
#define KdBreakPoint() DbgBreakPoint()
#define KdBreakPointWithStatus(s) DbgBreakPointWithStatus(s)

#else /* !DBG */

#define KdPrint(_x_)
#define KdPrintEx(_x_)
#define vKdPrintEx(_x_)
#define vKdPrintExWithPrefix(_x_)
#define KdBreakPoint()
#define KdBreakPointWithStatus(s)

#endif /* !DBG */

#if defined(__GNUC__)

extern NTKERNELAPI BOOLEAN KdDebuggerNotPresent;
extern NTKERNELAPI BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent

#elif defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_WDMDDK_) || defined(_NTOSP_)

extern NTKERNELAPI PBOOLEAN KdDebuggerNotPresent;
extern NTKERNELAPI PBOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED *KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT *KdDebuggerNotPresent

#else

extern BOOLEAN KdDebuggerNotPresent;
extern BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent

#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
KdDisableDebugger(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
KdEnableDebugger(VOID);

#if (_MSC_FULL_VER >= 150030729) && !defined(IMPORT_NATIVE_DBG_BREAK)
#define DbgBreakPoint __debugbreak
#else
VOID
NTAPI
DbgBreakPoint(VOID);
#endif

NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
  IN ULONG Status);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WS03)
NTKERNELAPI
BOOLEAN
NTAPI
KdRefreshDebuggerNotPresent(VOID);
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
NTKERNELAPI
NTSTATUS
NTAPI
KdChangeOption(
  IN KD_OPTION Option,
  IN ULONG InBufferBytes OPTIONAL,
  IN PVOID InBuffer,
  IN ULONG OutBufferBytes OPTIONAL,
  OUT PVOID OutBuffer,
  OUT PULONG OutBufferNeeded OPTIONAL);
#endif
$endif

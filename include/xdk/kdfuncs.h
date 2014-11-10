/******************************************************************************
 *                          Kernel Debugger Functions                         *
 ******************************************************************************/
$if (_NTDDK_)
NTSYSAPI
ULONG
NTAPI
DbgPrompt(
  _In_z_ PCCH Prompt,
  _Out_writes_bytes_(MaximumResponseLength) PCH Response,
  _In_ ULONG MaximumResponseLength);
$endif (_NTDDK_)

$if (_WDMDDK_)
#ifndef _DBGNT_

ULONG
__cdecl
DbgPrint(
  _In_z_ _Printf_format_string_ PCSTR Format,
  ...);

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONG
__cdecl
DbgPrintReturnControlC(
  _In_z_ _Printf_format_string_ PCCH Format,
  ...);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
  _In_ ULONG ComponentId,
  _In_ ULONG Level,
  _In_z_ _Printf_format_string_ PCSTR Format,
  ...);

#ifdef _VA_LIST_DEFINED

NTSYSAPI
ULONG
NTAPI
vDbgPrintEx(
  _In_ ULONG ComponentId,
  _In_ ULONG Level,
  _In_z_ PCCH Format,
  _In_ va_list ap);

NTSYSAPI
ULONG
NTAPI
vDbgPrintExWithPrefix(
  _In_z_ PCCH Prefix,
  _In_ ULONG ComponentId,
  _In_ ULONG Level,
  _In_z_ PCCH Format,
  _In_ va_list ap);

#endif /* _VA_LIST_DEFINED */

NTSYSAPI
NTSTATUS
NTAPI
DbgQueryDebugFilterState(
  _In_ ULONG ComponentId,
  _In_ ULONG Level);

NTSYSAPI
NTSTATUS
NTAPI
DbgSetDebugFilterState(
  _In_ ULONG ComponentId,
  _In_ ULONG Level,
  _In_ BOOLEAN State);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef VOID
(*PDEBUG_PRINT_CALLBACK)(
  _In_ PSTRING Output,
  _In_ ULONG ComponentId,
  _In_ ULONG Level);

NTSYSAPI
NTSTATUS
NTAPI
DbgSetDebugPrintCallback(
  _In_ PDEBUG_PRINT_CALLBACK DebugPrintCallback,
  _In_ BOOLEAN Enable);

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

#ifdef _NTSYSTEM_
extern BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED KdDebuggerEnabled
extern BOOLEAN KdDebuggerNotPresent;
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(KdDebuggerEnabled)
extern BOOLEAN *KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED (*KdDebuggerEnabled)
__CREATE_NTOS_DATA_IMPORT_ALIAS(KdDebuggerNotPresent)
extern BOOLEAN *KdDebuggerNotPresent;
#define KD_DEBUGGER_NOT_PRESENT (*KdDebuggerNotPresent)
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
__analysis_noreturn
VOID
NTAPI
DbgBreakPoint(VOID);
#endif

__analysis_noreturn
NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    _In_ ULONG Status);

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
  _In_ KD_OPTION Option,
  _In_opt_ ULONG InBufferBytes,
  _In_ PVOID InBuffer,
  _In_opt_ ULONG OutBufferBytes,
  _Out_ PVOID OutBuffer,
  _Out_opt_ PULONG OutBufferNeeded);
#endif
$endif (_WDMDDK_)

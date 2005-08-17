/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/umfuncs.h
 * PURPOSE:         Prototypes for NT Library Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */
#ifndef _UMFUNCS_H
#define _UMFUNCS_H

/* DEPENDENCIES **************************************************************/

/* PROTOTYPES ****************************************************************/

/*
 * CSR Functions
 */
NTSTATUS
STDCALL
CsrClientConnectToServer(
    PWSTR ObjectDirectory,
    ULONG ServerId,
    PVOID Unknown,
    PVOID Context,
    ULONG ContextLength,
    PBOOLEAN ServerToServerCall
);

struct _CSR_API_MESSAGE;
NTSTATUS
STDCALL
CsrClientCallServer(
    struct _CSR_API_MESSAGE *Request,
    PVOID CapturedBuffer OPTIONAL,
    ULONG ApiNumber,
    ULONG RequestLength
);

NTSTATUS
STDCALL
CsrIdentifyAlertableThread(VOID);

NTSTATUS
STDCALL
CsrNewThread(VOID);

NTSTATUS
STDCALL
CsrSetPriorityClass(
    HANDLE Process,
    PULONG PriorityClass
);

VOID
STDCALL
CsrProbeForRead(
    IN CONST PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
);

VOID
STDCALL
CsrProbeForWrite(
    IN CONST PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
);

NTSTATUS
STDCALL
CsrCaptureParameterBuffer(
    PVOID ParameterBuffer,
    ULONG ParameterBufferSize,
    PVOID* ClientAddress,
    PVOID* ServerAddress
);

NTSTATUS
STDCALL
CsrReleaseParameterBuffer(PVOID ClientAddress);

/*
 * Debug Functions
 */
ULONG
CDECL
DbgPrint(
    IN PCH  Format,
    IN ...
);

VOID
STDCALL
DbgBreakPoint(VOID);

NTSTATUS
STDCALL
DbgSsInitialize(
    HANDLE ReplyPort,
    PVOID Callback,
    ULONG Unknown2,
    ULONG Unknown3
);

NTSTATUS
STDCALL
DbgUiConnectToDbg(VOID);

NTSTATUS
STDCALL
DbgUiContinue(
    PCLIENT_ID ClientId,
    ULONG ContinueStatus
);

NTSTATUS
STDCALL
DbgUiWaitStateChange(
    ULONG Unknown1,
    ULONG Unknown2
);

VOID
STDCALL
DbgUiRemoteBreakin(VOID);

NTSTATUS
STDCALL
DbgUiIssueRemoteBreakin(HANDLE Process);

/*
 * Loader Functions
 */
NTSTATUS
STDCALL
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress);

NTSTATUS
STDCALL
LdrGetDllHandle(
    IN PWCHAR Path OPTIONAL,
    IN ULONG Unknown2,
    IN PUNICODE_STRING DllName,
    OUT PVOID *BaseAddress
);

NTSTATUS
STDCALL
LdrFindEntryForAddress(
    IN PVOID Address,
    OUT PLDR_DATA_TABLE_ENTRY *Module
);

NTSTATUS
STDCALL
LdrGetProcedureAddress(
    IN PVOID BaseAddress,
    IN PANSI_STRING Name,
    IN ULONG Ordinal,
    OUT PVOID *ProcedureAddress
);

VOID
STDCALL
LdrInitializeThunk(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3,
    ULONG Unknown4
);

NTSTATUS
STDCALL
LdrLoadDll(
    IN PWSTR SearchPath OPTIONAL,
    IN ULONG LoadFlags,
    IN PUNICODE_STRING Name,
    OUT PVOID *BaseAddress OPTIONAL
);

PIMAGE_BASE_RELOCATION
STDCALL
LdrProcessRelocationBlock(
    IN PVOID Address,
    IN USHORT Count,
    IN PUSHORT TypeOffset,
    IN ULONG_PTR Delta
);

NTSTATUS
STDCALL
LdrQueryImageFileExecutionOptions(
    IN PUNICODE_STRING SubKey,
    IN PCWSTR ValueName,
    IN ULONG ValueSize,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG RetunedLength OPTIONAL
);

NTSTATUS
STDCALL
LdrQueryProcessModuleInformation(
    IN PMODULE_INFORMATION ModuleInformation OPTIONAL,
    IN ULONG Size OPTIONAL,
    OUT PULONG ReturnedSize
);

NTSTATUS
STDCALL
LdrShutdownProcess(VOID);

NTSTATUS
STDCALL
LdrShutdownThread(VOID);

NTSTATUS
STDCALL
LdrUnloadDll(IN PVOID BaseAddress);

NTSTATUS
STDCALL
LdrVerifyImageMatchesChecksum(
    IN HANDLE FileHandle,
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

#endif
/* EOF */

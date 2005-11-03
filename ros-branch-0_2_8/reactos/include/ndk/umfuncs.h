/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/umfuncs.h
 * PURPOSE:         Prototypes for NT Library Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */
#ifndef _UMFUNCS_H
#define _UMFUNCS_H

/* DEPENDENCIES **************************************************************/
struct _CSR_API_MESSAGE;
struct _CSR_CAPTURE_BUFFER;

/* PROTOTYPES ****************************************************************/

/*
 * CSR Functions
 */
PVOID
NTAPI
CsrAllocateCaptureBuffer(
    ULONG ArgumentCount,
    ULONG BufferSize
);

ULONG
NTAPI
CsrAllocateMessagePointer(
    struct _CSR_CAPTURE_BUFFER *CaptureBuffer,
	ULONG MessageLength,
	PVOID *CaptureData
);

VOID
NTAPI
CsrCaptureMessageBuffer(
    struct _CSR_CAPTURE_BUFFER *CaptureBuffer,
	PVOID MessageString,
	ULONG StringLength,
	PVOID *CapturedData
);

NTSTATUS
NTAPI
CsrClientConnectToServer(
    PWSTR ObjectDirectory,
    ULONG ServerId,
    PVOID ConnectionInfo,
    PULONG ConnectionInfoSize,
    PBOOLEAN ServerToServerCall
);

NTSTATUS
NTAPI
CsrClientCallServer(
    struct _CSR_API_MESSAGE *Request,
    struct _CSR_CAPTURE_BUFFER *CaptureBuffer OPTIONAL,
    ULONG ApiNumber,
    ULONG RequestLength
);

NTSTATUS
NTAPI
CsrIdentifyAlertableThread(VOID);

VOID
NTAPI
CsrFreeCaptureBuffer(struct _CSR_CAPTURE_BUFFER *CaptureBuffer);

NTSTATUS
NTAPI
CsrNewThread(VOID);

NTSTATUS
NTAPI
CsrSetPriorityClass(
    HANDLE Process,
    PULONG PriorityClass
);

VOID
NTAPI
CsrProbeForRead(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
);

VOID
NTAPI
CsrProbeForWrite(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
);

/*
 * Debug Functions
 */
ULONG
__cdecl
DbgPrint(
    IN PCH  Format,
    IN ...
);

VOID
NTAPI
DbgBreakPoint(VOID);

NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(IN ULONG Status);

NTSTATUS
NTAPI
DbgUiConnectToDbg(VOID);

NTSTATUS
NTAPI
DbgUiContinue(
    PCLIENT_ID ClientId,
    ULONG ContinueStatus
);

NTSTATUS
NTAPI
DbgUiWaitStateChange(
    PDBGUI_WAIT_STATE_CHANGE DbgUiWaitStateCange,
    PLARGE_INTEGER TimeOut
);

VOID
NTAPI
DbgUiRemoteBreakin(VOID);

NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(HANDLE Process);

/*
 * Loader Functions
 */
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress);

NTSTATUS
NTAPI
LdrGetDllHandle(
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics,
    IN PUNICODE_STRING DllName,
    OUT PVOID *DllHandle
);

NTSTATUS
NTAPI
LdrFindEntryForAddress(
    IN PVOID Address,
    OUT PLDR_DATA_TABLE_ENTRY *Module
);

NTSTATUS
NTAPI
LdrGetProcedureAddress(
    IN PVOID BaseAddress,
    IN PANSI_STRING Name,
    IN ULONG Ordinal,
    OUT PVOID *ProcedureAddress
);

VOID
NTAPI
LdrInitializeThunk(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3,
    ULONG Unknown4
);

NTSTATUS
NTAPI
LdrLoadDll(
    IN PWSTR SearchPath OPTIONAL,
    IN ULONG LoadFlags,
    IN PUNICODE_STRING Name,
    OUT PVOID *BaseAddress OPTIONAL
);

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    IN PVOID Address,
    IN USHORT Count,
    IN PUSHORT TypeOffset,
    IN ULONG_PTR Delta
);

NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(
    IN PUNICODE_STRING SubKey,
    IN PCWSTR ValueName,
    IN ULONG ValueSize,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG RetunedLength OPTIONAL
);

NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(
    IN PMODULE_INFORMATION ModuleInformation OPTIONAL,
    IN ULONG Size OPTIONAL,
    OUT PULONG ReturnedSize
);

NTSTATUS
NTAPI
LdrShutdownProcess(VOID);

NTSTATUS
NTAPI
LdrShutdownThread(VOID);

NTSTATUS
NTAPI
LdrUnloadDll(IN PVOID BaseAddress);

NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    IN HANDLE FileHandle,
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

#endif
/* EOF */

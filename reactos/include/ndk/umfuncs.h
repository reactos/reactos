/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    umfuncs.h

Abstract:

    Function definitions for Native DLL (ntdll) APIs exclusive to User Mode.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _UMFUNCS_H
#define _UMFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <dbgktypes.h>

//
// Don't force inclusion of csrss header, leave this opaque.
//
struct _CSR_API_MESSAGE;
struct _CSR_CAPTURE_BUFFER;

//
// CSR Functions
//
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
CsrIdentifyAlertableThread(
    VOID
);

VOID
NTAPI
CsrFreeCaptureBuffer(
    struct _CSR_CAPTURE_BUFFER *CaptureBuffer
);

HANDLE
NTAPI
CsrGetProcessId(
    VOID
);

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

//
// Debug Functions
//
NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    IN ULONG Status
);

NTSTATUS
NTAPI
DbgUiConnectToDbg(
    VOID
);

NTSTATUS
NTAPI
DbgUiContinue(
    IN PCLIENT_ID ClientId,
    IN NTSTATUS ContinueStatus
);

NTSTATUS
NTAPI
DbgUiDebugActiveProcess(
    IN HANDLE Process
);

NTSTATUS
NTAPI
DbgUiStopDebugging(
    IN HANDLE Process
);

NTSTATUS
NTAPI
DbgUiWaitStateChange(
    IN PDBGUI_WAIT_STATE_CHANGE DbgUiWaitStateCange,
    IN PLARGE_INTEGER TimeOut
);

NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(
    IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
    IN PVOID DebugEvent
);

VOID
NTAPI
DbgUiRemoteBreakin(
    VOID
);

NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(
    IN HANDLE Process
);

HANDLE
NTAPI
DbgUiGetThreadDebugObject(
    VOID
);

//
// Loader Functions
//

NTSTATUS
NTAPI
LdrAddRefDll(
    IN ULONG Flags,
    IN PVOID BaseAddress
);

NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    IN PVOID BaseAddress
);

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
    IN PULONG LoadFlags OPTIONAL,
    IN PUNICODE_STRING Name,
    OUT PVOID *BaseAddress OPTIONAL
);

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    IN ULONG_PTR Address,
    IN ULONG Count,
    IN PUSHORT TypeOffset,
    IN LONG_PTR Delta
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
    IN PRTL_PROCESS_MODULES ModuleInformation OPTIONAL,
    IN ULONG Size OPTIONAL,
    OUT PULONG ReturnedSize
);

NTSTATUS
NTAPI
LdrShutdownProcess(
    VOID
);

NTSTATUS
NTAPI
LdrShutdownThread(
    VOID
);

NTSTATUS
NTAPI
LdrUnloadDll(
    IN PVOID BaseAddress
);

NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    IN HANDLE FileHandle,
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

#endif

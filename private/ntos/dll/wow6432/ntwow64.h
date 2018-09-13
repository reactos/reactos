/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntwow64.h

Abstract:

    This module contains headers for fake kernel entrypoints(wow64 BOPS) in ntdll.  

Author:

    Michael Zoran (mzoran) 22-NOV-1998

Environment:

    User Mode only

Revision History:

--*/

NTSYSAPI
NTSTATUS
NTAPI
NtWow64CsrClientConnectToServer(
    IN PWSTR ObjectDirectory,
    IN ULONG ServerDllIndex,
    IN PCSR_CALLBACK_INFO CallbackInformation OPTIONAL,
    IN PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWow64CsrNewThread(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWow64CsrIdentifyAlertableThread( 
    VOID 
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWow64CsrClientCallServer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer OPTIONAL,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    );

NTSYSAPI
PCSR_CAPTURE_HEADER
NTAPI
NtWow64CsrAllocateCaptureBuffer(
    IN ULONG CountMessagePointers,
    IN ULONG Size
    );

NTSYSAPI
VOID
NTAPI
NtWow64CsrFreeCaptureBuffer(
    IN PCSR_CAPTURE_HEADER CaptureBuffer
    );

NTSYSAPI
ULONG
NTAPI
NtWow64CsrAllocateMessagePointer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN ULONG Length,
    OUT PVOID *Pointer
    );

NTSYSAPI
VOID
NTAPI
NtWow64CsrCaptureMessageBuffer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    OUT PVOID *CapturedBuffer
    );

NTSYSAPI
VOID
NTAPI
NtWow64CsrCaptureMessageString(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PCSTR String OPTIONAL,
    IN ULONG Length,
    IN ULONG MaximumLength,
    OUT PSTRING CapturedString
    );

NTSYSAPI
VOID
NTAPI
NtWow64CsrCaptureMessageString(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PCSTR String OPTIONAL,
    IN ULONG Length,
    IN ULONG MaximumLength,
    OUT PSTRING CapturedString
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWow64CsrSetPriorityClass(
    IN HANDLE ProcessHandle,
    IN OUT PULONG PriorityClass
    );


NTSYSAPI
NTSTATUS
NTAPI
NtDbgUiConnectToDbg( VOID );

NTSTATUS
NtDbgUiWaitStateChange (
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
NtDbgUiContinue (
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
    );

// This is used in place of INT 2D
NTSYSAPI
NTSTATUS
NtWow64DebuggerCall (
    IN ULONG ServiceClass,
    IN ULONG Arg1,
    IN ULONG Arg2
    );

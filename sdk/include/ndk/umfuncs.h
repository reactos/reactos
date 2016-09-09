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
// Debug Functions
//
__analysis_noreturn
NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    _In_ ULONG Status
);

NTSTATUS
NTAPI
DbgUiConnectToDbg(
    VOID
);

NTSTATUS
NTAPI
DbgUiContinue(
    _In_ PCLIENT_ID ClientId,
    _In_ NTSTATUS ContinueStatus
);

NTSTATUS
NTAPI
DbgUiDebugActiveProcess(
    _In_ HANDLE Process
);

NTSTATUS
NTAPI
DbgUiStopDebugging(
    _In_ HANDLE Process
);

NTSYSAPI
NTSTATUS
NTAPI
DbgUiWaitStateChange(
    _In_ PDBGUI_WAIT_STATE_CHANGE DbgUiWaitStateCange,
    _In_ PLARGE_INTEGER TimeOut
);

NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(
    _In_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
    _In_ PVOID DebugEvent
);

VOID
NTAPI
DbgUiRemoteBreakin(
    VOID
);

NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(
    _In_ HANDLE Process
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
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress
);

NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID BaseAddress
);

NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PWSTR DllPath,
    _In_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
);

NTSTATUS
NTAPI
LdrGetDllHandleEx(
    _In_ ULONG Flags,
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_opt_ PVOID *DllHandle);

NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID Address,
    _Out_ PLDR_DATA_TABLE_ENTRY *Module
);

NTSTATUS
NTAPI
LdrGetProcedureAddress(
    _In_ PVOID BaseAddress,
    _In_ PANSI_STRING Name,
    _In_ ULONG Ordinal,
    _Out_ PVOID *ProcedureAddress
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
    _In_opt_ PWSTR SearchPath,
    _In_opt_ PULONG LoadFlags,
    _In_ PUNICODE_STRING Name,
    _Out_opt_ PVOID *BaseAddress
);

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    _In_ ULONG_PTR Address,
    _In_ ULONG Count,
    _In_ PUSHORT TypeOffset,
    _In_ LONG_PTR Delta
);

NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(
    _In_ PUNICODE_STRING SubKey,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueSize,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG RetunedLength
);

NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(
    _In_opt_ PRTL_PROCESS_MODULES ModuleInformation,
    _In_opt_ ULONG Size,
    _Out_ PULONG ReturnedSize
);

VOID
NTAPI
LdrSetDllManifestProber(
    _In_ PLDR_MANIFEST_PROBER_ROUTINE Routine);

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
    _In_ PVOID BaseAddress
);

typedef VOID (NTAPI *PLDR_CALLBACK)(PVOID CallbackContext, PCHAR Name);
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    _In_ HANDLE FileHandle,
    _In_ PLDR_CALLBACK Callback,
    _In_ PVOID CallbackContext,
    _Out_ PUSHORT ImageCharacterstics
);

NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(
    _In_ PUNICODE_STRING SubKey,
    _In_ BOOLEAN Wow64,
    _Out_ PHANDLE NewKeyHandle
);

NTSTATUS
NTAPI
LdrQueryImageFileKeyOption(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength
);

#endif

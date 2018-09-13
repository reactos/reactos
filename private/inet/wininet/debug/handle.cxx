/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    handle.cxx

Abstract:

    Contains function to return number of open handles owned by this process

    Contents:
        InternetHandleCount

Author:

    Richard L Firth (rfirth) 02-May-1995

Environment:

    Win32 user-mode DLL

Revision History:

    02-May-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include "rprintf.h"

#if INET_DEBUG

//
// private types
//

typedef (*NT_QUERY_SYSTEM_INFORMATION)(ULONG, PVOID, ULONG, PULONG);

//
// functions
//


DWORD
InternetHandleCount(
    VOID
    )

/*++

Routine Description:

    Gets the number of system handles owned by this process. We LoadLibrary()
    NTDLL.DLL so that the debug version of this DLL still works on Win95

Arguments:

    None.

Return Value:

    DWORD

--*/

{
    static HINSTANCE hNtdll = NULL;
    static NT_QUERY_SYSTEM_INFORMATION _NtQuerySystemInformation;

    if (IsPlatformWin95()) {
        return 0;
    }

    if (hNtdll == NULL) {
        hNtdll = LoadLibrary("ntdll");
        if (hNtdll == NULL) {
            return 0;
        }
        _NtQuerySystemInformation = (NT_QUERY_SYSTEM_INFORMATION)GetProcAddress(hNtdll, "NtQuerySystemInformation");
        if (_NtQuerySystemInformation == 0) {
            FreeLibrary(hNtdll);
            hNtdll = NULL;
        }
    }

    if (_NtQuerySystemInformation) {

        DWORD idProcess;
        NTSTATUS status;
        ULONG outputLength;
        BYTE buffer[32768];
        PSYSTEM_PROCESS_INFORMATION info;

        status = _NtQuerySystemInformation(SystemProcessInformation,
                                           (PVOID)buffer,
                                           sizeof(buffer),
                                           &outputLength
                                           );
        if (!NT_SUCCESS(status)) {
            return 0;
        }
        info = (PSYSTEM_PROCESS_INFORMATION)buffer;
        idProcess = GetCurrentProcessId();
        while (TRUE) {
            if ((DWORD_PTR)info->UniqueProcessId == idProcess) {
                return info->HandleCount;
            }
            if (info->NextEntryOffset == 0) {
                return 0;
            }
            info = (PSYSTEM_PROCESS_INFORMATION)((PCHAR)info + info->NextEntryOffset);
        }
    }
    return 0;
}

#endif // INET_DEBUG

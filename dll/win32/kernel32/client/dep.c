/*
 * PROJECT:     ReactOS Kernel32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Data Execution Prevention (DEP) policy functions
 * COPYRIGHT:   Copyright 2010 Detlef Riekenberg
 *              Copyright 2011 Austin English
 *              Copyright 2026 Mark Jansen <mark.jansen@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
DEP_SYSTEM_POLICY_TYPE
WINAPI
GetSystemDEPPolicy(VOID)
{
    return (DEP_SYSTEM_POLICY_TYPE)SharedUserData->NXSupportPolicy;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessDEPPolicy(
    _In_ HANDLE hProcess,
    _Out_opt_ LPDWORD lpFlags,
    _Out_opt_ PBOOL lpPermanent)
{
    ULONG ExecuteFlags;
    NTSTATUS Status;

    /* ProcessExecuteFlags is only valid for the current process */
    UNREFERENCED_PARAMETER(hProcess);

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessExecuteFlags,
                                       &ExecuteFlags,
                                       sizeof(ExecuteFlags),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (lpFlags)
    {
        *lpFlags = 0;
        if (ExecuteFlags & MEM_EXECUTE_OPTION_DISABLE)
            *lpFlags |= PROCESS_DEP_ENABLE;
        if (ExecuteFlags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            *lpFlags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
    }

    if (lpPermanent)
        *lpPermanent = (ExecuteFlags & MEM_EXECUTE_OPTION_PERMANENT) != 0;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetProcessDEPPolicy(
    _In_ DWORD dwFlags)
{
    ULONG ExecuteFlags = 0;
    NTSTATUS Status;

    if (dwFlags & PROCESS_DEP_ENABLE)
        ExecuteFlags |= MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_PERMANENT;
    if (dwFlags & PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION)
        ExecuteFlags |= MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION;

    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessExecuteFlags,
                                     &ExecuteFlags,
                                     sizeof(ExecuteFlags));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

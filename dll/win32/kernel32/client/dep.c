/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            dll/win32/kernel32/client/dep.c
 * PURPOSE:         ReactOS Data Execution Prevention (DEP) functions
 * COPYRIGHT:       Copyright 2021 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief Retrieves the data execution prevention (DEP) and thunk emulation (DEP-ATL)
 * settings for the process.
 * 
 * On Windows XP SP3, the current process is handled.
 * On Vista and newer, the process specified in hProcess parameter is handled.
 * 
 * @param [in] hProcess
 * A handle to a process with PROCESS_QUERY_INFORMATION privilege.
 * Ignored on Windows XP SP3.
 * 
 * @param [out] lpFlags
 * Pointer to a variable that receives the flags:
 * 0 - DEP is disabled.
 * PROCESS_DEP_ENABLE - DEP is enabled.
 * PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION - DEP-ATL is disabled.
 * 
 * @param [out] lpPermanent
 * Pointer to a variable that specifies whether DEP is enabled or disabled permanently.
 * 
 * @return
 * TRUE on success, FALSE on failure.
 * 
 * @remarks
 * Supported for 32-bit processes only. Returns ERROR_NOT_SUPPORTED for 64-bit processes.
 * 
 */
BOOL
WINAPI
GetProcessDEPPolicy(_In_ HANDLE hProcess,
                    _Out_ LPDWORD lpFlags,
                    _Out_ PBOOL lpPermanent)
{
    ULONG Flags;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(GetCurrentProcess(),
                                       ProcessExecuteFlags,
                                       &Flags,
                                       sizeof(Flags),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (lpFlags)
    {
        *lpFlags = 0;
        if (Flags & MEM_EXECUTE_OPTION_DISABLE)
            *lpFlags |= PROCESS_DEP_ENABLE;
        if (Flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            *lpFlags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
    }

    if (lpPermanent)
        *lpPermanent = (Flags & MEM_EXECUTE_OPTION_PERMANENT) != 0;

    return TRUE;
}

/**
 * @brief Sets the data execution prevention (DEP) and thunk emulation (DEP-ATL)
 * settings for the current process.
 * 
 * @param [in] dwFlags
 * Variable that specifies the flags to set:
 * 0 - disables DEP;
 * PROCESS_DEP_ENABLE - enables DEP permanently.
 * PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION - disables DEP-ATL. Can be specified
 *                                           only with PROCESS_DEP_ENABLE.
 * 
 * @return
 * TRUE on success, FALSE on failure.
 * 
 * @remarks
 * Supported for 32-bit processes only. Returns ERROR_NOT_SUPPORTED for 64-bit processes.
 * 
 */
BOOL
WINAPI
SetProcessDEPPolicy(_In_ DWORD dwFlags)
{
    ULONG Flags = 0;
    NTSTATUS Status;

    if (dwFlags & PROCESS_DEP_ENABLE)
        Flags |= MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_PERMANENT;
    if (dwFlags & PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION)
        Flags |= MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION;

    Status = NtSetInformationProcess(GetCurrentProcess(),
                                     ProcessExecuteFlags,
                                     &Flags,
                                     sizeof(Flags));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Retrieves the system data execution prevention (DEP) and thunk emulation (DEP-ATL)
 * settings.
 * 
 * @return
 * One of DEP_SYSTEM_POLICY_TYPE enumeration values:
 * AlwaysOff - DEP is always disabled system-wide.
 * AlwaysOn - DEP is always enabled system-wide.
 * OptIn - DEP is enabled only for system components.
 *         Default for client Windows versions.
 * OptOut - DEP is enabled for system components and processes.
 *          Default for server Windows versions.
 * 
 * @remarks
 * Supported for 32-bit processes only.
 * 
 */
DEP_SYSTEM_POLICY_TYPE
WINAPI
GetSystemDEPPolicy(VOID)
{
    return SharedUserData->NXSupportPolicy;
}

/* EOF */

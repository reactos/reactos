/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/handle.c
 * PURPOSE:         Object Handle Functions
 * PROGRAMMERS:     Ariadne ( ariadne@xs4all.nl)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

HANDLE
TranslateStdHandle(IN HANDLE hHandle)
{
    PRTL_USER_PROCESS_PARAMETERS Ppb = NtCurrentPeb()->ProcessParameters;

    switch ((ULONG)hHandle)
    {
        case STD_INPUT_HANDLE:  return Ppb->StandardInput;
        case STD_OUTPUT_HANDLE: return Ppb->StandardOutput;
        case STD_ERROR_HANDLE:  return Ppb->StandardError;
    }

    return hHandle;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetHandleInformation(IN HANDLE hObject,
                     OUT LPDWORD lpdwFlags)
{
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
    ULONG BytesWritten;
    NTSTATUS Status;
    DWORD Flags;

    hObject = TranslateStdHandle(hObject);

    if (IsConsoleHandle(hObject))
    {
        return GetConsoleHandleInformation(hObject, lpdwFlags);
    }

    Status = NtQueryObject(hObject,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
                           &BytesWritten);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    Flags = 0;
    if (HandleInfo.Inherit) Flags |= HANDLE_FLAG_INHERIT;
    if (HandleInfo.ProtectFromClose) Flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
    *lpdwFlags = Flags;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetHandleInformation(IN HANDLE hObject,
                     IN DWORD dwMask,
                     IN DWORD dwFlags)
{
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
    ULONG BytesWritten;
    NTSTATUS Status;

    hObject = TranslateStdHandle(hObject);

    if (IsConsoleHandle(hObject))
    {
        return SetConsoleHandleInformation(hObject, dwMask, dwFlags);
    }

    Status = NtQueryObject(hObject,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
                           &BytesWritten);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (dwMask & HANDLE_FLAG_INHERIT)
    {
        HandleInfo.Inherit = (dwFlags & HANDLE_FLAG_INHERIT) != 0;
    }

    if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
    {
        HandleInfo.ProtectFromClose = (dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;
    }

    Status = NtSetInformationObject(hObject,
                                    ObjectHandleFlagInformation,
                                    &HandleInfo,
                                    sizeof(HandleInfo));
    if (NT_SUCCESS(Status)) return TRUE;

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CloseHandle(IN HANDLE hObject)
{
    NTSTATUS Status;

    hObject = TranslateStdHandle(hObject);

    if (IsConsoleHandle(hObject)) return CloseConsoleHandle(hObject);

    Status = NtClose(hObject);
    if (NT_SUCCESS(Status)) return TRUE;

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DuplicateHandle(IN HANDLE hSourceProcessHandle,
                IN HANDLE hSourceHandle,
                IN HANDLE hTargetProcessHandle,
                OUT LPHANDLE lpTargetHandle,
                IN DWORD dwDesiredAccess,
                IN BOOL bInheritHandle,
                IN DWORD dwOptions)
{
    NTSTATUS Status;
    HANDLE hTargetHandle;

    hSourceHandle = TranslateStdHandle(hSourceHandle);

    if ((IsConsoleHandle(hSourceHandle)) &&
        ((hSourceHandle != NtCurrentProcess()) &&
         (hSourceHandle != NtCurrentThread())))
    {
        /*
         * We can duplicate console handles only if both the source
         * and the target processes are in fact the current process.
         */
        if ((hSourceProcessHandle != NtCurrentProcess()) ||
            (hTargetProcessHandle != NtCurrentProcess()))
        {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
        }

        hTargetHandle = DuplicateConsoleHandle(hSourceHandle,
                                               dwDesiredAccess,
                                               bInheritHandle,
                                               dwOptions);
        if (hTargetHandle != INVALID_HANDLE_VALUE)
        {
            if (lpTargetHandle) *lpTargetHandle = hTargetHandle;
            return TRUE;
        }

        return FALSE;
    }

    Status = NtDuplicateObject(hSourceProcessHandle,
                               hSourceHandle,
                               hTargetProcessHandle,
                               lpTargetHandle,
                               dwDesiredAccess,
                               bInheritHandle ? OBJ_INHERIT : 0,
                               dwOptions);
    if (NT_SUCCESS(Status)) return TRUE;

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
UINT
WINAPI
SetHandleCount(IN UINT nCount)
{
    return nCount;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetHandleContext(IN HANDLE Handle)
{
    /* This is Windows behavior, not a ReactOS Stub */
    DbgPrintEx(0, 0, "Unsupported API - kernel32!GetHandleContext() called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSocketHandle(VOID)
{
    /* This is Windows behavior, not a ReactOS Stub */
    DbgPrintEx(0, 0, "Unsupported API - kernel32!CreateSocketHandle() called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetHandleContext(IN HANDLE Handle,
                 IN DWORD Context)
{
    /* This is Windows behavior, not a ReactOS Stub */
    DbgPrintEx(0, 0, "Unsupported API - kernel32!SetHandleContext() called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/* EOF */

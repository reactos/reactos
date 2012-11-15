/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/history.c
 * PURPOSE:         Win32 Console Client history functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/

static BOOL
IntExpungeConsoleCommandHistory(LPCVOID lpExeName, BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status;

    if (lpExeName == NULL || !(bUnicode ? *(PWCHAR)lpExeName : *(PCHAR)lpExeName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(1, IntStringSize(lpExeName, bUnicode));
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    IntCaptureMessageString(CaptureBuffer, lpExeName, bUnicode,
                            &Request.Data.ExpungeCommandHistory.ExeName);

    Status = CsrClientCallServer(&Request,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, EXPUNGE_COMMAND_HISTORY),
                                 sizeof(CSR_API_MESSAGE));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


static DWORD
IntGetConsoleCommandHistory(LPVOID lpHistory, DWORD cbHistory, LPCVOID lpExeName, BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status;
    DWORD HistoryLength = cbHistory * (bUnicode ? 1 : sizeof(WCHAR));

    if (lpExeName == NULL || !(bUnicode ? *(PWCHAR)lpExeName : *(PCHAR)lpExeName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(2, IntStringSize(lpExeName, bUnicode) +
                                                HistoryLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    IntCaptureMessageString(CaptureBuffer, lpExeName, bUnicode,
                            &Request.Data.GetCommandHistory.ExeName);
    Request.Data.GetCommandHistory.Length = HistoryLength;
    CsrAllocateMessagePointer(CaptureBuffer, HistoryLength,
                              (PVOID*)&Request.Data.GetCommandHistory.History);

    Status = CsrClientCallServer(&Request,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_COMMAND_HISTORY),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(Status);
        return 0;
    }

    if (bUnicode)
    {
        memcpy(lpHistory,
               Request.Data.GetCommandHistory.History,
               Request.Data.GetCommandHistory.Length);
    }
    else
    {
        WideCharToMultiByte(CP_ACP, 0,
                            Request.Data.GetCommandHistory.History,
                            Request.Data.GetCommandHistory.Length / sizeof(WCHAR),
                            lpHistory,
                            cbHistory,
                            NULL, NULL);
    }

    CsrFreeCaptureBuffer(CaptureBuffer);
    return Request.Data.GetCommandHistory.Length;
}


static DWORD
IntGetConsoleCommandHistoryLength(LPCVOID lpExeName, BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status;

    if (lpExeName == NULL || !(bUnicode ? *(PWCHAR)lpExeName : *(PCHAR)lpExeName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(1, IntStringSize(lpExeName, bUnicode));
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    IntCaptureMessageString(CaptureBuffer, lpExeName, bUnicode,
                            &Request.Data.GetCommandHistoryLength.ExeName);

    Status = CsrClientCallServer(&Request,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_COMMAND_HISTORY_LENGTH),
                                 sizeof(CSR_API_MESSAGE));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return Request.Data.GetCommandHistoryLength.Length;
}


static BOOL
IntSetConsoleNumberOfCommands(DWORD dwNumCommands,
                              LPCVOID lpExeName,
                              BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status;

    if (lpExeName == NULL || !(bUnicode ? *(PWCHAR)lpExeName : *(PCHAR)lpExeName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(1, IntStringSize(lpExeName, bUnicode));
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    IntCaptureMessageString(CaptureBuffer, lpExeName, bUnicode,
                            &Request.Data.SetHistoryNumberCommands.ExeName);
    Request.Data.SetHistoryNumberCommands.NumCommands = dwNumCommands;

    Status = CsrClientCallServer(&Request,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, SET_HISTORY_NUMBER_COMMANDS),
                                 sizeof(CSR_API_MESSAGE));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/* FUNCTIONS ******************************************************************/

/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
ExpungeConsoleCommandHistoryW(LPCWSTR lpExeName)
{
    return IntExpungeConsoleCommandHistory(lpExeName, TRUE);
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
ExpungeConsoleCommandHistoryA(LPCSTR lpExeName)
{
    return IntExpungeConsoleCommandHistory(lpExeName, FALSE);
}


/*
 * @implemented (Undocumented)
 */
DWORD
WINAPI
GetConsoleCommandHistoryW(LPWSTR lpHistory,
                          DWORD cbHistory,
                          LPCWSTR lpExeName)
{
    return IntGetConsoleCommandHistory(lpHistory, cbHistory, lpExeName, TRUE);
}


/*
 * @implemented (Undocumented)
 */
DWORD
WINAPI
GetConsoleCommandHistoryA(LPSTR lpHistory,
                          DWORD cbHistory,
                          LPCSTR lpExeName)
{
    return IntGetConsoleCommandHistory(lpHistory, cbHistory, lpExeName, FALSE);
}


/*
 * @implemented (Undocumented)
 */
DWORD
WINAPI
GetConsoleCommandHistoryLengthW(LPCWSTR lpExeName)
{
    return IntGetConsoleCommandHistoryLength(lpExeName, TRUE);
}


/*
 * @implemented (Undocumented)
 */
DWORD
WINAPI
GetConsoleCommandHistoryLengthA(LPCSTR lpExeName)
{
    return IntGetConsoleCommandHistoryLength(lpExeName, FALSE) / sizeof(WCHAR);
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleNumberOfCommandsW(DWORD dwNumCommands,
                            LPCSTR lpExeName)
{
    return IntSetConsoleNumberOfCommands(dwNumCommands, lpExeName, TRUE);
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleNumberOfCommandsA(DWORD dwNumCommands,
                            LPCWSTR lpExeName)
{
    return IntSetConsoleNumberOfCommands(dwNumCommands, lpExeName, FALSE);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetConsoleCommandHistoryMode(IN DWORD dwMode)
{
    STUB;
    return FALSE;
}

/* EOF */

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

/* Get the size needed to copy a string to a capture buffer, including alignment */
static ULONG
IntStringSize(LPCVOID String,
              BOOL Unicode)
{
    ULONG Size = (Unicode ? wcslen(String) : strlen(String)) * sizeof(WCHAR);
    return (Size + 3) & -4;
}


/* Copy a string to a capture buffer */
static VOID
IntCaptureMessageString(PCSR_CAPTURE_BUFFER CaptureBuffer,
                        LPCVOID String,
                        BOOL Unicode,
                        PUNICODE_STRING RequestString)
{
    ULONG Size;
    if (Unicode)
    {
        Size = wcslen(String) * sizeof(WCHAR);
        CsrCaptureMessageBuffer(CaptureBuffer, (PVOID)String, Size, (PVOID *)&RequestString->Buffer);
    }
    else
    {
        Size = strlen(String);
        CsrAllocateMessagePointer(CaptureBuffer, Size * sizeof(WCHAR), (PVOID *)&RequestString->Buffer);
        Size = MultiByteToWideChar(CP_ACP, 0, String, Size, RequestString->Buffer, Size * sizeof(WCHAR))
               * sizeof(WCHAR);
    }
    RequestString->Length = RequestString->MaximumLength = (USHORT)Size;
}


static BOOL
IntExpungeConsoleCommandHistory(LPCVOID lpExeName, BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_EXPUNGECOMMANDHISTORY ExpungeCommandHistoryRequest = &ApiMessage.Data.ExpungeCommandHistoryRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

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
                            &ExpungeCommandHistoryRequest->ExeName);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepExpungeCommandHistory),
                                 sizeof(CONSOLE_EXPUNGECOMMANDHISTORY));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


static DWORD
IntGetConsoleCommandHistory(LPVOID lpHistory, DWORD cbHistory, LPCVOID lpExeName, BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETCOMMANDHISTORY GetCommandHistoryRequest = &ApiMessage.Data.GetCommandHistoryRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
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
                            &GetCommandHistoryRequest->ExeName);
    GetCommandHistoryRequest->Length = HistoryLength;
    CsrAllocateMessagePointer(CaptureBuffer, HistoryLength,
                              (PVOID*)&GetCommandHistoryRequest->History);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCommandHistory),
                                 sizeof(CONSOLE_GETCOMMANDHISTORY));
    if (!NT_SUCCESS(Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(Status);
        return 0;
    }

    if (bUnicode)
    {
        memcpy(lpHistory,
               GetCommandHistoryRequest->History,
               GetCommandHistoryRequest->Length);
    }
    else
    {
        WideCharToMultiByte(CP_ACP, 0,
                            GetCommandHistoryRequest->History,
                            GetCommandHistoryRequest->Length / sizeof(WCHAR),
                            lpHistory,
                            cbHistory,
                            NULL, NULL);
    }

    CsrFreeCaptureBuffer(CaptureBuffer);

    return GetCommandHistoryRequest->Length;
}


static DWORD
IntGetConsoleCommandHistoryLength(LPCVOID lpExeName, BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETCOMMANDHISTORYLENGTH GetCommandHistoryLengthRequest = &ApiMessage.Data.GetCommandHistoryLengthRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

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
                            &GetCommandHistoryLengthRequest->ExeName);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCommandHistoryLength),
                                 sizeof(CONSOLE_GETCOMMANDHISTORYLENGTH));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return GetCommandHistoryLengthRequest->Length;
}


static BOOL
IntSetConsoleNumberOfCommands(DWORD dwNumCommands,
                              LPCVOID lpExeName,
                              BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETHISTORYNUMBERCOMMANDS SetHistoryNumberCommandsRequest = &ApiMessage.Data.SetHistoryNumberCommandsRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

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
                            &SetHistoryNumberCommandsRequest->ExeName);
    SetHistoryNumberCommandsRequest->NumCommands = dwNumCommands;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetNumberOfCommands),
                                 sizeof(CONSOLE_SETHISTORYNUMBERCOMMANDS));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status))
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

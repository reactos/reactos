/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Vista functions
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@reactos.com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* PUBLIC FUNCTIONS ***********************************************************/

#if _WIN32_WINNT >= 0x600

/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleHistoryInfo(PCONSOLE_HISTORY_INFO lpConsoleHistoryInfo)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHISTORYINFO HistoryInfoRequest = &ApiMessage.Data.HistoryInfoRequest;

    if (lpConsoleHistoryInfo->cbSize != sizeof(CONSOLE_HISTORY_INFO))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetHistory),
                                 sizeof(CONSOLE_GETSETHISTORYINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    lpConsoleHistoryInfo->HistoryBufferSize      = HistoryInfoRequest->HistoryBufferSize;
    lpConsoleHistoryInfo->NumberOfHistoryBuffers = HistoryInfoRequest->NumberOfHistoryBuffers;
    lpConsoleHistoryInfo->dwFlags                = HistoryInfoRequest->dwFlags;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetConsoleHistoryInfo(IN PCONSOLE_HISTORY_INFO lpConsoleHistoryInfo)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHISTORYINFO HistoryInfoRequest = &ApiMessage.Data.HistoryInfoRequest;

    if (lpConsoleHistoryInfo->cbSize != sizeof(CONSOLE_HISTORY_INFO))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    HistoryInfoRequest->HistoryBufferSize      = lpConsoleHistoryInfo->HistoryBufferSize;
    HistoryInfoRequest->NumberOfHistoryBuffers = lpConsoleHistoryInfo->NumberOfHistoryBuffers;
    HistoryInfoRequest->dwFlags                = lpConsoleHistoryInfo->dwFlags;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetHistory),
                                 sizeof(CONSOLE_GETSETHISTORYINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetConsoleOriginalTitleW(OUT LPWSTR lpConsoleTitle,
                         IN DWORD nSize)
{
    DPRINT1("GetConsoleOriginalTitleW(0x%p, 0x%x) UNIMPLEMENTED!\n", lpConsoleTitle, nSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetConsoleOriginalTitleA(OUT LPSTR lpConsoleTitle,
                         IN DWORD nSize)
{
    DPRINT1("GetConsoleOriginalTitleA(0x%p, 0x%x) UNIMPLEMENTED!\n", lpConsoleTitle, nSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetConsoleScreenBufferInfoEx(IN HANDLE hConsoleOutput,
                             OUT PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx)
{
    DPRINT1("GetConsoleScreenBufferInfoEx(0x%p, 0x%p) UNIMPLEMENTED!\n", hConsoleOutput, lpConsoleScreenBufferInfoEx);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetConsoleScreenBufferInfoEx(IN HANDLE hConsoleOutput,
                             IN PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx)
{
    DPRINT1("SetConsoleScreenBufferInfoEx(0x%p, 0x%p) UNIMPLEMENTED!\n", hConsoleOutput, lpConsoleScreenBufferInfoEx);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetCurrentConsoleFontEx(IN HANDLE hConsoleOutput,
                        IN BOOL bMaximumWindow,
                        OUT PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx)
{
    DPRINT1("GetCurrentConsoleFontEx(0x%p, 0x%x, 0x%p) UNIMPLEMENTED!\n", hConsoleOutput, bMaximumWindow, lpConsoleCurrentFontEx);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif

/* EOF */

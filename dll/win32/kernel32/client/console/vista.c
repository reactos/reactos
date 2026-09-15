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

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetConsoleHistoryInfo(PCONSOLE_HISTORY_INFO lpConsoleHistoryInfo)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHISTORYINFO HistoryInfoRequest = &ApiMessage.Data.HistoryInfoRequest;

    if (lpConsoleHistoryInfo->cbSize != sizeof(CONSOLE_HISTORY_INFO))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetHistory),
                        sizeof(*HistoryInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
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
DECLSPEC_HOTPATCH
SetConsoleHistoryInfo(IN PCONSOLE_HISTORY_INFO lpConsoleHistoryInfo)
{
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

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetHistory),
                        sizeof(*HistoryInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
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
DECLSPEC_HOTPATCH
GetConsoleOriginalTitleA(OUT LPSTR lpConsoleTitle,
                         IN DWORD nSize)
{
    DPRINT1("GetConsoleOriginalTitleA(0x%p, 0x%x) UNIMPLEMENTED!\n", lpConsoleTitle, nSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetConsoleScreenBufferInfoEx(IN HANDLE hConsoleOutput,
                             OUT PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSCREENBUFFERINFOEX ScreenBufferInfoExRequest = &ApiMessage.Data.ScreenBufferInfoExRequest;

    if (lpConsoleScreenBufferInfoEx == NULL ||
        lpConsoleScreenBufferInfoEx->cbSize != sizeof(CONSOLE_SCREEN_BUFFER_INFOEX))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ScreenBufferInfoExRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ScreenBufferInfoExRequest->OutputHandle  = hConsoleOutput;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage, NULL, CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetScreenBufferInfoEx), sizeof(*ScreenBufferInfoExRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    lpConsoleScreenBufferInfoEx->dwSize              = ScreenBufferInfoExRequest->ScreenBufferSize;
    lpConsoleScreenBufferInfoEx->dwCursorPosition    = ScreenBufferInfoExRequest->CursorPosition;
    lpConsoleScreenBufferInfoEx->wAttributes         = ScreenBufferInfoExRequest->Attributes;
    lpConsoleScreenBufferInfoEx->srWindow.Left       = ScreenBufferInfoExRequest->ViewOrigin.X;
    lpConsoleScreenBufferInfoEx->srWindow.Top        = ScreenBufferInfoExRequest->ViewOrigin.Y;
    lpConsoleScreenBufferInfoEx->srWindow.Right      = ScreenBufferInfoExRequest->ViewOrigin.X + ScreenBufferInfoExRequest->ViewSize.X - 1;
    lpConsoleScreenBufferInfoEx->srWindow.Bottom     = ScreenBufferInfoExRequest->ViewOrigin.Y + ScreenBufferInfoExRequest->ViewSize.Y - 1;
    lpConsoleScreenBufferInfoEx->dwMaximumWindowSize = ScreenBufferInfoExRequest->MaximumViewSize;
    lpConsoleScreenBufferInfoEx->wPopupAttributes    = ScreenBufferInfoExRequest->PopupAttributes;
    lpConsoleScreenBufferInfoEx->bFullscreenSupported = ScreenBufferInfoExRequest->FullscreenSupported;

    RtlCopyMemory(lpConsoleScreenBufferInfoEx->ColorTable, ScreenBufferInfoExRequest->ColorTable, sizeof(ScreenBufferInfoExRequest->ColorTable));

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleScreenBufferInfoEx(IN HANDLE hConsoleOutput,
                             IN PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSCREENBUFFERINFOEX ScreenBufferInfoExRequest = &ApiMessage.Data.ScreenBufferInfoExRequest;

    if (lpConsoleScreenBufferInfoEx == NULL ||
        lpConsoleScreenBufferInfoEx->cbSize != sizeof(CONSOLE_SCREEN_BUFFER_INFOEX))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ScreenBufferInfoExRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ScreenBufferInfoExRequest->OutputHandle  = hConsoleOutput;

    ScreenBufferInfoExRequest->ScreenBufferSize = lpConsoleScreenBufferInfoEx->dwSize;
    ScreenBufferInfoExRequest->CursorPosition   = lpConsoleScreenBufferInfoEx->dwCursorPosition;
    ScreenBufferInfoExRequest->Attributes       = lpConsoleScreenBufferInfoEx->wAttributes;
    ScreenBufferInfoExRequest->ViewOrigin.X     = lpConsoleScreenBufferInfoEx->srWindow.Left;
    ScreenBufferInfoExRequest->ViewOrigin.Y     = lpConsoleScreenBufferInfoEx->srWindow.Top;
    ScreenBufferInfoExRequest->ViewSize.X       = lpConsoleScreenBufferInfoEx->srWindow.Right - lpConsoleScreenBufferInfoEx->srWindow.Left + 1;
    ScreenBufferInfoExRequest->ViewSize.Y       = lpConsoleScreenBufferInfoEx->srWindow.Bottom - lpConsoleScreenBufferInfoEx->srWindow.Top + 1;
    ScreenBufferInfoExRequest->MaximumViewSize  = lpConsoleScreenBufferInfoEx->dwMaximumWindowSize;
    ScreenBufferInfoExRequest->PopupAttributes  = lpConsoleScreenBufferInfoEx->wPopupAttributes;
    ScreenBufferInfoExRequest->FullscreenSupported = lpConsoleScreenBufferInfoEx->bFullscreenSupported;

    RtlCopyMemory(ScreenBufferInfoExRequest->ColorTable, lpConsoleScreenBufferInfoEx->ColorTable, sizeof(ScreenBufferInfoExRequest->ColorTable));

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage, NULL, CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetScreenBufferInfoEx), sizeof(*ScreenBufferInfoExRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetCurrentConsoleFontEx(IN HANDLE hConsoleOutput,
                        IN BOOL bMaximumWindow,
                        OUT PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx)
{
    DPRINT1("GetCurrentConsoleFontEx(0x%p, 0x%x, 0x%p) UNIMPLEMENTED!\n", hConsoleOutput, bMaximumWindow, lpConsoleCurrentFontEx);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

/* EOF */

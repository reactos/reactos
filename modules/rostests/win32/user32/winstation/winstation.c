/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests some aspects of the Window Station API.
 * COPYRIGHT:   Copyright 2018 Hermes Belusca-Maito
 */

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>

#include <strsafe.h>

#include "resource.h"


/*
 * Logging facilities
 */

typedef struct _LOG_FILE
{
    HANDLE  hLogFile;
    PWCHAR  pBuffer;
    size_t  cbBufferSize;
} LOG_FILE, *PLOG_FILE;

BOOL
InitLog(
    OUT PLOG_FILE LogFile,
    IN LPCWSTR LogFileName,
    IN PWCHAR pBuffer,
    IN size_t cbBufferSize)
{
    HANDLE hLogFile;

    ZeroMemory(LogFile, sizeof(*LogFile));

    hLogFile = CreateFileW(LogFileName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (hLogFile == INVALID_HANDLE_VALUE)
        return FALSE;

    LogFile->hLogFile = hLogFile;
    LogFile->pBuffer  = pBuffer;
    LogFile->cbBufferSize = cbBufferSize;

    return TRUE;
}

VOID
CloseLog(
    IN PLOG_FILE LogFile)
{
    CloseHandle(LogFile->hLogFile);
    ZeroMemory(LogFile, sizeof(*LogFile));
}

BOOL
WriteToLog(
    IN PLOG_FILE LogFile,
    IN LPCVOID Buffer,
    IN DWORD dwBufferSize)
{
    return WriteFile(LogFile->hLogFile,
                     Buffer,
                     dwBufferSize,
                     &dwBufferSize, NULL);
}

BOOL
WriteToLogPuts(
    IN PLOG_FILE LogFile,
    IN LPCWSTR String)
{
    return WriteToLog(LogFile,
                      String,
                      wcslen(String) * sizeof(WCHAR));
}

BOOL
WriteToLogPrintfV(
    IN PLOG_FILE LogFile,
    IN LPCWSTR Format,
    IN va_list args)
{
    StringCbVPrintfW(LogFile->pBuffer,
                     LogFile->cbBufferSize,
                     Format, args);

    return WriteToLog(LogFile,
                      LogFile->pBuffer,
                      wcslen(LogFile->pBuffer) * sizeof(WCHAR));
}

BOOL
WriteToLogPrintf(
    IN PLOG_FILE LogFile,
    IN LPCWSTR Format,
    ...)
{
    BOOL bRet;
    va_list args;

    va_start(args, Format);
    bRet = WriteToLogPrintfV(LogFile, Format, args);
    va_end(args);

    return bRet;
}


/*
 * Window Station tests
 */

BOOL
CALLBACK
EnumDesktopProc(
    IN LPWSTR lpszDesktop,
    IN LPARAM lParam)
{
    PLOG_FILE LogFile = (PLOG_FILE)lParam;

    WriteToLogPrintf(LogFile, L" :: Found desktop '%s'\r\n", lpszDesktop);

    /* Continue the enumeration */
    return TRUE;
}

/*
 * This test inspects the same window station aspects that are used in the
 * Cygwin fhandler_console.cc!fhandler_console::create_invisible_console()
 * function, see:
 *   https://github.com/cygwin/cygwin/blob/7b9bfb4136f23655e243bab89fb62b04bdbacc7f/winsup/cygwin/fhandler_console.cc#L2494
 */
VOID DoTest(HWND hWnd)
{
    HWINSTA hWinSta;
    LPCWSTR lpszWinSta = L"Test-WinSta";
    BOOL bIsItOk;
    LOG_FILE LogFile;
    WCHAR szBuffer[2048];

    bIsItOk = InitLog(&LogFile, L"test_winsta.log", szBuffer, sizeof(szBuffer));
    if (!bIsItOk)
    {
        MessageBoxW(hWnd, L"Could not create the log file, stopping test now...", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    /* Switch output to UTF-16 (little endian) */
    WriteToLog(&LogFile, "\xFF\xFE", 2);

    WriteToLogPrintf(&LogFile, L"Creating Window Station '%s'\r\n", lpszWinSta);
    hWinSta = CreateWindowStationW(lpszWinSta, 0, WINSTA_ALL_ACCESS, NULL);
    WriteToLogPrintf(&LogFile, L"--> Returned handle 0x%p ; last error: %lu\r\n", hWinSta, GetLastError());

    if (!hWinSta)
    {
        WriteToLogPuts(&LogFile, L"\r\nHandle is NULL, cannot proceed further, stopping the test!\r\n\r\n");
        return;
    }

    WriteToLogPrintf(&LogFile, L"Enumerate desktops on Window Station '%s' (0x%p) (before process attach)\r\n", lpszWinSta, hWinSta);
    bIsItOk = EnumDesktopsW(hWinSta, EnumDesktopProc, (LPARAM)&LogFile);
    WriteToLogPrintf(&LogFile, L"--> Returned %s ; last error: %lu\r\n",
                     (bIsItOk ? L"success" : L"failure"), GetLastError());

    WriteToLogPrintf(&LogFile, L"Setting current process to Window Station '%s' (0x%p)\r\n", lpszWinSta, hWinSta);
    bIsItOk = SetProcessWindowStation(hWinSta);
    WriteToLogPrintf(&LogFile, L"--> Returned %s ; last error: %lu\r\n",
                     (bIsItOk ? L"success" : L"failure"), GetLastError());

    WriteToLogPrintf(&LogFile, L"Enumerate desktops on Window Station '%s' (0x%p) (after process attach, before allocating console)\r\n", lpszWinSta, hWinSta);
    bIsItOk = EnumDesktopsW(hWinSta, EnumDesktopProc, (LPARAM)&LogFile);
    WriteToLogPrintf(&LogFile, L"--> Returned %s ; last error: %lu\r\n",
                     (bIsItOk ? L"success" : L"failure"), GetLastError());

    WriteToLogPrintf(&LogFile, L"Allocating a new console on Window Station '%s' (0x%p)\r\n", lpszWinSta, hWinSta);
    bIsItOk = AllocConsole();
    WriteToLogPrintf(&LogFile, L"--> Returned %s ; last error: %lu\r\n",
                     (bIsItOk ? L"success" : L"failure"), GetLastError());

    WriteToLogPrintf(&LogFile, L"Enumerate desktops on Window Station '%s' (0x%p) (after allocating console)\r\n", lpszWinSta, hWinSta);
    bIsItOk = EnumDesktopsW(hWinSta, EnumDesktopProc, (LPARAM)&LogFile);
    WriteToLogPrintf(&LogFile, L"--> Returned %s ; last error: %lu\r\n",
                     (bIsItOk ? L"success" : L"failure"), GetLastError());

    WriteToLogPrintf(&LogFile, L"Now closing Window Station '%s' (0x%p)\r\n", lpszWinSta, hWinSta);
    bIsItOk = CloseWindowStation(hWinSta);
    WriteToLogPrintf(&LogFile, L"--> Returned %s ; last error: %lu\r\n\r\n",
                     (bIsItOk ? L"success" : L"failure"), GetLastError());

    CloseLog(&LogFile);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
                DoTest(hDlg);
                EndDialog(hDlg, LOWORD(wParam));
                break;

            case IDCANCEL:
            default:
                EndDialog(hDlg, LOWORD(wParam));
                break;
        }
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_ABOUTBOX), NULL, About);
}

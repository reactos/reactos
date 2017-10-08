/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/tcpsvcs/log.c
 * PURPOSE:     Logging functionality for the service
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

#define DEBUG

static LPWSTR lpEventSource = L"tcpsvcs";
static LPCWSTR lpLogFileName = L"C:\\tcpsvcs_log.log";
static HANDLE hLogFile = NULL;

static OVERLAPPED olWrite;


// needs work
static VOID
LogToEventLog(LPCWSTR lpMsg,
              DWORD errNum,
              DWORD exitCode,
              UINT flags)
{
    HANDLE hEventLog;

    hEventLog = RegisterEventSourceW(NULL, lpEventSource);
    if (hEventLog)
    {
        ReportEventW(hEventLog,
                     (flags & LOG_ERROR) ? EVENTLOG_ERROR_TYPE : EVENTLOG_SUCCESS,
                     0,
                     0,
                     NULL,
                     1,
                     0,
                     &lpMsg,
                     NULL);

        CloseEventLog(hEventLog);
    }
}

static BOOL
OpenLogFile()
{
    hLogFile = CreateFileW(lpLogFileName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL);
    if (hLogFile  == INVALID_HANDLE_VALUE)
    {
        hLogFile = NULL;
        return FALSE;
    }

    return TRUE;
}

static VOID
LogToFile(LPCWSTR lpMsg,
          DWORD errNum,
          DWORD exitCode,
          UINT flags)
{
    LPWSTR lpFullMsg = NULL;
    DWORD msgLen;

    msgLen = wcslen(lpMsg) + 1;

    if (flags & LOG_ERROR)
    {
        LPVOID lpSysMsg;
        DWORD eMsgLen;

        eMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 errNum,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (LPTSTR)&lpSysMsg,
                                 0,
                                 NULL);

        msgLen = msgLen + eMsgLen + 40;

        lpFullMsg = HeapAlloc(GetProcessHeap(),
                              0,
                              msgLen * sizeof(TCHAR));
        if (lpFullMsg)
        {
            _snwprintf(lpFullMsg,
                       msgLen,
                       L"%s : %s\tErrNum = %lu ExitCode = %lu\r\n",
                       lpMsg,
                       lpSysMsg,
                       errNum,
                       exitCode);
        }

        LocalFree(lpSysMsg);

    }
    else
    {
        msgLen += 2;

        lpFullMsg = HeapAlloc(GetProcessHeap(),
                              0,
                              msgLen * sizeof(TCHAR));
        if (lpFullMsg)
        {
            _snwprintf(lpFullMsg,
                       msgLen,
                      L"%s\r\n",
                      lpMsg);
        }
    }

    if (lpFullMsg)
    {
        DWORD bytesWritten;
        DWORD dwRet;
        BOOL bRet;

        bRet = WriteFile(hLogFile,
                         lpFullMsg,
                         wcslen(lpFullMsg) * sizeof(WCHAR),
                         &bytesWritten,
                         &olWrite);
        if (!bRet)
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                bRet = FALSE;
            }
            else
            {
                // Write is pending
                dwRet = WaitForSingleObject(olWrite.hEvent, INFINITE);

                 switch (dwRet)
                 {
                    // event has been signaled
                    case WAIT_OBJECT_0:
                    {
                         bRet = GetOverlappedResult(hLogFile,
                                                    &olWrite,
                                                    &bytesWritten,
                                                    FALSE);
                         break;
                    }

                    default:
                         // An error has occurred in WaitForSingleObject.
                         // This usually indicates a problem with the
                         // OVERLAPPED structure's event handle.
                         bRet = FALSE;
                         break;
                 }
            }
        }

        if (!bRet || bytesWritten == 0)
        {
            LogToEventLog(L"Failed to write to log file",
                          GetLastError(),
                          0,
                          LOG_EVENTLOG | LOG_ERROR);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpFullMsg);
    }

    if (exitCode > 0)
        ExitProcess(exitCode);
}



VOID
LogEvent(LPCWSTR lpMsg,
         DWORD errNum,
         DWORD exitCode,
         UINT flags)
{
#ifdef DEBUG
    if (flags & LOG_FILE || flags & LOG_ERROR)
        LogToFile(lpMsg, errNum, exitCode, flags);
#endif
    if (flags & LOG_EVENTLOG)
        LogToEventLog(lpMsg, errNum, exitCode, flags);
}

BOOL
InitLogging()
{
#ifdef DEBUG
    BOOL bRet = FALSE;

    ZeroMemory(&olWrite, sizeof(OVERLAPPED));
    olWrite.Offset = 0xFFFFFFFF;
    olWrite.OffsetHigh = 0xFFFFFFFF;
    olWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (olWrite.hEvent)
    {
        DeleteFileW(lpLogFileName);

        if (OpenLogFile())
        {
            WCHAR wcBom = 0xFEFF;
            DWORD bytesWritten;

            bRet = WriteFile(hLogFile,
                             &wcBom,
                             sizeof(WCHAR),
                             &bytesWritten,
                             &olWrite);
            if (!bRet)
            {
                if (GetLastError() != ERROR_IO_PENDING)
                {
                    LogToEventLog(L"Failed to write to log file",
                                  GetLastError(),
                                  0,
                                  LOG_EVENTLOG | LOG_ERROR);
                }
                else
                {
                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
#else
    return TRUE;
#endif
}

VOID
UninitLogging()
{
    if (hLogFile)
    {
        FlushFileBuffers(hLogFile);
        CloseHandle(hLogFile);
    }

    if (olWrite.hEvent)
    {
        CloseHandle(olWrite.hEvent);
    }
}

/*
 * PROJECT:     ReactOS services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:
 * PURPOSE:     skeleton service
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "myservice.h"

static LPTSTR lpEventSource = _T("Skeleton service");
static LPTSTR lpLogFileName = _T("C:\\skel_service.log");
static HANDLE hLogFile;

// needs work
static VOID
LogToEventLog(LPCTSTR lpMsg,
              DWORD errNum,
              DWORD exitCode,
              UINT flags)
{
    HANDLE hEventLog;

    hEventLog = RegisterEventSource(NULL, lpEventSource);
    if (hEventLog)
    {
        ReportEvent(hEventLog,
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
    hLogFile = CreateFile(lpLogFileName,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    if (hLogFile  == INVALID_HANDLE_VALUE)
        return FALSE;

    return TRUE;
}

static BOOL
LogToFile(LPCTSTR lpMsg,
          DWORD errNum,
          DWORD exitCode,
          UINT flags)
{
    LPTSTR lpFullMsg = NULL;
    DWORD msgLen;

    if (!OpenLogFile())
        return FALSE;

    msgLen = _tcslen(lpMsg) + 1;

    if (flags & LOG_ERROR)
    {
        LPVOID lpSysMsg;
        DWORD eMsgLen;

        eMsgLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
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
            _sntprintf(lpFullMsg,
                       msgLen,
                       _T("%s : %s\tErrNum = %lu ExitCode = %lu\r\n"),
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
            _sntprintf(lpFullMsg,
                       msgLen,
                      _T("%s\r\n"),
                      lpMsg);
        }
    }

    if (lpFullMsg)
    {
        DWORD bytesWritten;

        SetFilePointer(hLogFile, 0, NULL, FILE_END);

        WriteFile(hLogFile,
                  lpFullMsg,
                  _tcslen(lpFullMsg) * sizeof(TCHAR),
                  &bytesWritten,
                  NULL);
        if (bytesWritten == 0)
        {
            LogToEventLog(_T("Failed to write to log file"),
                          GetLastError(),
                          0,
                          LOG_EVENTLOG | LOG_ERROR);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpFullMsg);
    }

    CloseHandle(hLogFile);

    if (exitCode > 0)
        ExitProcess(exitCode);
}


VOID
LogEvent(LPCTSTR lpMsg,
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


VOID
InitLogging()
{
    WCHAR wcBom = 0xFEFF;

    DeleteFile(lpLogFileName);

#ifdef _UNICODE
    if (OpenLogFile())
    {
        DWORD bytesWritten;

        WriteFile(hLogFile,
                  &wcBom,
                  sizeof(WCHAR),
                  &bytesWritten,
                  NULL);
        if (bytesWritten == 0)
        {
            LogToEventLog(_T("Failed to write to log file"),
                          GetLastError(),
                          0,
                          LOG_EVENTLOG | LOG_ERROR);
        }

        CloseHandle(hLogFile);
    }
#endif
}

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    perfdiag.cxx

Abstract:

    Performance diagnostics

    Contents:
        WininetPerfLog
        PerfSleep
        PerfSelect
        PerfWaitForSingleObject
        (CPerfDiag::get_next_record)
        CPerfDiag::CPerfDiag
        CPerfDiag::~CPerfDiag
        CPerfDiag::Log(DWORD, DWORD)
        CPerfDiag::Log(DWORD, DWORD, DWORD, HINTERNET)
        CPerfDiag::Log(DWORD, DWORD, DWORD, DWORD, HINTERNET)
        CPerfDiag::Dump
        (map_perf_event)
        (map_callback_status)
        (map_async_request)
        (map_thread_pri)
        (map_length)

Author:

    Richard L Firth (rfirth) 24-Jan-1997

Revision History:

    24-Jan-1997 rfirth
        Created

--*/

#include <wininetp.h>

#if defined(USE_PERF_DIAG)

#include <perfdiag.hxx>

//
// global data
//

GLOBAL CPerfDiag * GlobalPerfDiag = NULL;
GLOBAL BOOL GlobalDumpPerfToFile = TRUE;

//
// private prototypes
//

PRIVATE LPSTR map_perf_event(DWORD dwEvent);
PRIVATE LPSTR map_callback_status(DWORD dwStatus);
PRIVATE LPSTR map_async_request(DWORD dwRequest);
PRIVATE LPSTR map_thread_pri(DWORD dwPriority);
PRIVATE LPSTR map_length(DWORD dwLength, LPSTR lpBuf);

//
// APIs
//

INTERNETAPI
VOID
WINAPI
WininetPerfLog(
    IN DWORD dwEvent,
    IN DWORD dwInfo1,
    IN DWORD dwInfo2,
    IN HINTERNET hInternet
    ) {
    if (!GlobalPerfDiag) {
        GlobalPerfDiag = new CPerfDiag;
    }
    if (GlobalPerfDiag) {
        GlobalPerfDiag->Log(dwEvent, dwInfo1, dwInfo2, GetCurrentThreadId(), hInternet);
    }
}

//
// functions
//

VOID PerfSleep(DWORD dwMilliseconds) {
    PERF_LOG(PE_YIELD_SLEEP_START);
    Sleep(dwMilliseconds);
    PERF_LOG(PE_YIELD_SLEEP_END);
}

int PerfSelect(int nfds,	
    fd_set FAR * readfds,	
    fd_set FAR * writefds,	
    fd_set FAR * exceptfds,	
    const struct timeval FAR * timeout 	
    ) {
    PERF_LOG(PE_YIELD_SELECT_START);

    int n = _I_select(nfds, readfds, writefds, exceptfds, timeout);

    PERF_LOG(PE_YIELD_SELECT_END);
    return n;
}

DWORD PerfWaitForSingleObject(
    HANDLE hObject,
    DWORD dwTimeout
    ) {
    PERF_LOG(PE_YIELD_OBJECT_WAIT_START);

    DWORD result = WaitForSingleObject(hObject, dwTimeout);

    PERF_LOG(PE_YIELD_OBJECT_WAIT_END);

    return result;
}

//
// private methods
//

LPPERF_INFO CPerfDiag::get_next_record(VOID) {

    if (!m_lpbPerfBuffer || m_bFull) {
        return NULL;
    }

    LPBYTE lpbCurrent;
    LPBYTE lpbNext;
    LPBYTE result;

    do {
        lpbCurrent = m_lpbNext;
        lpbNext = lpbCurrent + sizeof(PERF_INFO);
        result = (LPBYTE)InterlockedExchangePointer((PVOID*)&m_lpbNext, lpbNext);
    } while ((result != (LPBYTE)lpbCurrent) && (lpbCurrent < m_lpbEnd));
    if (lpbCurrent >= m_lpbEnd) {
        m_bFull = TRUE;
        OutputDebugString("*** Wininet performance log is full!\n");
        lpbCurrent = NULL;
    }
    return (LPPERF_INFO)lpbCurrent;
}

//
// public methods
//

CPerfDiag::CPerfDiag() {
    m_lpbPerfBuffer = NULL;
    m_dwPerfBufferLen = 0;
    m_lpbEnd = NULL;
    m_lpbNext = NULL;
    m_bFull = FALSE;
    m_bStarted = FALSE;
    m_bStartFinished = FALSE;
    m_liStartTime.QuadPart = 0i64;
    perf_start();
}

CPerfDiag::~CPerfDiag() {
    free_perf_buffer();
}

VOID CPerfDiag::Log(DWORD dwEvent, DWORD dwInfo) {

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo) {
        Log(dwEvent, dwInfo, 0, lpThreadInfo->ThreadId, lpThreadInfo->hObject);
    }
}

VOID CPerfDiag::Log(DWORD dwEvent, DWORD dwInfo, DWORD dwThreadId, HINTERNET hInternet) {
    Log(dwEvent, dwInfo, 0, dwThreadId, hInternet);
}

VOID CPerfDiag::Log(DWORD dwEvent, DWORD dwInfo, DWORD dwInfo2, DWORD dwThreadId, HINTERNET hInternet) {

    //if (!m_bStarted) {
    //    perf_start();
    //}

    LPPERF_INFO lpInfo = get_next_record();

    if (!lpInfo) {
        return;
    }

    lpInfo->hInternet = hInternet;
    lpInfo->dwThreadId = dwThreadId;
    lpInfo->dwThreadPriority = GetThreadPriority(GetCurrentThread());
    lpInfo->dwEvent = dwEvent;
    lpInfo->dwInfo = dwInfo;
    lpInfo->dwInfo2 = dwInfo2;
    get_time(lpInfo);
}

VOID CPerfDiag::Dump(VOID) {

    HANDLE hFile = INVALID_HANDLE_VALUE;
    static const char PerfFileName[] = "WININET.PRF";

    if (GlobalDumpPerfToFile) {
        hFile = CreateFile((LPCSTR)PerfFileName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           INVALID_HANDLE_VALUE
                           );
        if (hFile == INVALID_HANDLE_VALUE) {
            OutputDebugString("failed to create perf file ");
            OutputDebugString((LPCSTR)PerfFileName);
            OutputDebugString("\n");
            GlobalDumpPerfToFile = FALSE;
        }
    }

    LARGE_INTEGER liFrequency;
    LONGLONG div1;
    LONGLONG div2;

    QueryPerformanceFrequency(&liFrequency);
    div1 = liFrequency.QuadPart;
    div2 = div1 / 1000000;

    LPPERF_INFO lpInfo;
    int record = 1;

    for (lpInfo = (LPPERF_INFO)m_lpbPerfBuffer; lpInfo != (LPPERF_INFO)m_lpbNext; ++lpInfo) {

        char buf[1024];
        LONGLONG ticks;
        DWORD microseconds;
        DWORD seconds;
        DWORD minutes;

        ticks = lpInfo->liTime.QuadPart - m_liStartTime.QuadPart;
        seconds = (DWORD)(ticks / div1);
        microseconds = (DWORD)((ticks % div1) / div2);

        //
        // don't understand why I have to do this? Win95 only (you could have guessed)
        // rounding error?
        //

        while (microseconds >= 1000000) {
            microseconds -= 1000000;
            ++seconds;
        }
        minutes = seconds / 60;
        seconds = seconds % 60;

        char lenbuf[32];
        char lenbuf2[32];

        int nChars = wsprintf(buf,
                              "%5d: Delta=%.2d:%.2d.%.6d TID=%08x Pri=%-8s hReq=%06x Info=%08x %-24s %-22s %s\r\n",
                              record,
                              minutes,
                              seconds,
                              microseconds,
                              lpInfo->dwThreadId,
                              map_thread_pri(lpInfo->dwThreadPriority),
                              lpInfo->hInternet,
                              lpInfo->dwInfo,
                              map_perf_event(lpInfo->dwEvent),
                              ((lpInfo->dwEvent == PE_APP_CALLBACK_START)
                              || (lpInfo->dwEvent == PE_APP_CALLBACK_END))
                                 ? map_callback_status(lpInfo->dwInfo)
                              : (((lpInfo->dwEvent == PE_WORKER_REQUEST_START)
                              || (lpInfo->dwEvent == PE_WORKER_REQUEST_END)
                              || (lpInfo->dwEvent == PE_CLIENT_REQUEST_START)
                              || (lpInfo->dwEvent == PE_CLIENT_REQUEST_END)
                              || (lpInfo->dwEvent == PE_CLIENT_REQUEST_QUEUED))
                                 ? map_async_request(lpInfo->dwInfo)
                              : (((lpInfo->dwEvent == PE_SEND_END)
                              || (lpInfo->dwEvent == PE_RECEIVE_END))
                                 ? map_length(lpInfo->dwInfo2, lenbuf)
                              : (((lpInfo->dwEvent == PE_ENTER_PATH)
                              || (lpInfo->dwEvent == PE_LEAVE_PATH)
                              || (lpInfo->dwEvent == PE_TRACE_PATH))
                                 ? (LPSTR)lpInfo->dwInfo2
                                 : ""))),
                              (((lpInfo->dwEvent == PE_CLIENT_REQUEST_END)
                              || (lpInfo->dwEvent == PE_WORKER_REQUEST_END))
                              && ((lpInfo->dwInfo == AR_INTERNET_READ_FILE)
                              || (lpInfo->dwInfo == AR_INTERNET_QUERY_DATA_AVAILABLE)))
                                 ? map_length(lpInfo->dwInfo2, lenbuf2)
                                 : ""
                             );
        if (GlobalDumpPerfToFile) {

            DWORD nWritten;

            WriteFile(hFile, buf, nChars, &nWritten, NULL);
        } else {
            OutputDebugString(buf);
        }
        ++record;
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}


PRIVATE LPSTR map_perf_event(DWORD dwEvent) {
    switch (dwEvent) {
    case PE_START:                      return "START";
    case PE_END:                        return "END";
    case PE_CLIENT_REQUEST_START:       return "CLIENT_REQUEST_START";
    case PE_CLIENT_REQUEST_END:         return "CLIENT_REQUEST_END";
    case PE_CLIENT_REQUEST_QUEUED:      return "CLIENT_REQUEST_QUEUED";
    case PE_WORKER_REQUEST_START:       return "WORKER_REQUEST_START";
    case PE_WORKER_REQUEST_END:         return "WORKER_REQUEST_END";
    case PE_APP_CALLBACK_START:         return "APP_CALLBACK_START";
    case PE_APP_CALLBACK_END:           return "APP_CALLBACK_END";
    case PE_NAMERES_START:              return "NAMERES_START";
    case PE_NAMERES_END:                return "NAMERES_END";
    case PE_CONNECT_START:              return "CONNECT_START";
    case PE_CONNECT_END:                return "CONNECT_END";
    case PE_SEND_START:                 return "SEND_START";
    case PE_SEND_END:                   return "SEND_END";
    case PE_RECEIVE_START:              return "RECEIVE_START";
    case PE_RECEIVE_END:                return "RECEIVE_END";
    case PE_PEEK_RECEIVE_START:         return "PEEK_RECEIVE_START";
    case PE_PEEK_RECEIVE_END:           return "PEEK_RECEIVE_END";
    case PE_SOCKET_CLOSE_START:         return "SOCKET_CLOSE_START";
    case PE_SOCKET_CLOSE_END:           return "SOCKET_CLOSE_END";
    case PE_ACQUIRE_KEEP_ALIVE:         return "ACQUIRE_KEEP_ALIVE";
    case PE_RELEASE_KEEP_ALIVE:         return "RELEASE_KEEP_ALIVE";
    case PE_SOCKET_ERROR:               return "SOCKET_ERROR";
    case PE_CACHE_READ_CHECK_START:     return "CACHE_READ_CHECK_START";
    case PE_CACHE_READ_CHECK_END:       return "CACHE_READ_CHECK_END";
    case PE_CACHE_WRITE_CHECK_START:    return "CACHE_WRITE_CHECK_START";
    case PE_CACHE_WRITE_CHECK_END:      return "CACHE_WRITE_CHECK_END";
    case PE_CACHE_RETRIEVE_START:       return "CACHE_RETRIEVE_START";
    case PE_CACHE_RETRIEVE_END:         return "CACHE_RETRIEVE_END";
    case PE_CACHE_READ_START:           return "CACHE_READ_START";
    case PE_CACHE_READ_END:             return "CACHE_READ_END";
    case PE_CACHE_WRITE_START:          return "CACHE_WRITE_START";
    case PE_CACHE_WRITE_END:            return "CACHE_WRITE_END";
    case PE_CACHE_CREATE_FILE_START:    return "CACHE_CREATE_FILE_START";
    case PE_CACHE_CREATE_FILE_END:      return "CACHE_CREATE_FILE_END";
    case PE_CACHE_CLOSE_FILE_START:     return "CACHE_CLOSE_FILE_START";
    case PE_CACHE_CLOSE_FILE_END:       return "CACHE_CLOSE_FILE_END";
    case PE_CACHE_EXPIRY_CHECK_START:   return "CACHE_EXPIRY_CHECK_START";
    case PE_CACHE_EXPIRY_CHECK_END:     return "CACHE_EXPIRY_CHECK_END";
    case PE_YIELD_SELECT_START:         return "YIELD_SELECT_START";
    case PE_YIELD_SELECT_END:           return "YIELD_SELECT_END";
    case PE_YIELD_OBJECT_WAIT_START:    return "YIELD_OBJECT_WAIT_START";
    case PE_YIELD_OBJECT_WAIT_END:      return "YIELD_OBJECT_WAIT_END";
    case PE_YIELD_SLEEP_START:          return "YIELD_SLEEP_START";
    case PE_YIELD_SLEEP_END:            return "YIELD_SLEEP_END";
    case PE_TRACE:                      return "TRACE";
    case PE_ENTER_PATH:                 return "ENTER_PATH";
    case PE_LEAVE_PATH:                 return "LEAVE_PATH";
    case PE_TRACE_PATH:                 return "TRACE_PATH";
    }
    return "?";
}

PRIVATE LPSTR map_callback_status(DWORD dwStatus) {
    switch (dwStatus) {
    case INTERNET_STATUS_RESOLVING_NAME:        return "RESOLVING_NAME";
    case INTERNET_STATUS_NAME_RESOLVED:         return "NAME_RESOLVED";
    case INTERNET_STATUS_CONNECTING_TO_SERVER:  return "CONNECTING_TO_SERVER";
    case INTERNET_STATUS_CONNECTED_TO_SERVER:   return "CONNECTED_TO_SERVER";
    case INTERNET_STATUS_SENDING_REQUEST:       return "SENDING_REQUEST";
    case INTERNET_STATUS_REQUEST_SENT:          return "REQUEST_SENT";
    case INTERNET_STATUS_RECEIVING_RESPONSE:    return "RECEIVING_RESPONSE";
    case INTERNET_STATUS_RESPONSE_RECEIVED:     return "RESPONSE_RECEIVED";
    case INTERNET_STATUS_CTL_RESPONSE_RECEIVED: return "CTL_RESPONSE_RECEIVED";
    case INTERNET_STATUS_PREFETCH:              return "PREFETCH";
    case INTERNET_STATUS_CLOSING_CONNECTION:    return "CLOSING_CONNECTION";
    case INTERNET_STATUS_CONNECTION_CLOSED:     return "CONNECTION_CLOSED";
    case INTERNET_STATUS_HANDLE_CREATED:        return "HANDLE_CREATED";
    case INTERNET_STATUS_HANDLE_CLOSING:        return "HANDLE_CLOSING";
    case INTERNET_STATUS_REQUEST_COMPLETE:      return "REQUEST_COMPLETE";
    case INTERNET_STATUS_REDIRECT:              return "REDIRECT";
    case INTERNET_STATUS_STATE_CHANGE:          return "STATE_CHANGE";
    }
    return "?";
}

PRIVATE LPSTR map_async_request(DWORD dwRequest) {
    switch (dwRequest) {
    case AR_INTERNET_CONNECT:               return "InternetConnect";
    case AR_INTERNET_OPEN_URL:              return "InternetOpenUrl";
    case AR_INTERNET_READ_FILE:             return "InternetReadFile";
    case AR_INTERNET_WRITE_FILE:            return "InternetWriteFile";
    case AR_INTERNET_QUERY_DATA_AVAILABLE:  return "InternetQueryDataAvailable";
    case AR_INTERNET_FIND_NEXT_FILE:        return "InternetFindNextFile";
    case AR_FTP_FIND_FIRST_FILE:            return "FtpFindFirstFile";
    case AR_FTP_GET_FILE:                   return "FtpGetFile";
    case AR_FTP_PUT_FILE:                   return "FtpPutFile";
    case AR_FTP_DELETE_FILE:                return "FtpDeleteFile";
    case AR_FTP_RENAME_FILE:                return "FtpRenameFile";
    case AR_FTP_OPEN_FILE:                  return "FtpOpenFile";
    case AR_FTP_CREATE_DIRECTORY:           return "FtpCreateDirectory";
    case AR_FTP_REMOVE_DIRECTORY:           return "FtpRemoveDirectory";
    case AR_FTP_SET_CURRENT_DIRECTORY:      return "FtpSetCurrentDirectory";
    case AR_FTP_GET_CURRENT_DIRECTORY:      return "FtpGetCurrentDirectory";
    case AR_GOPHER_FIND_FIRST_FILE:         return "GopherFindFirstFile";
    case AR_GOPHER_OPEN_FILE:               return "GopherOpenFile";
    case AR_GOPHER_GET_ATTRIBUTE:           return "GopherGetAttribute";
    case AR_HTTP_SEND_REQUEST:              return "HttpSendRequest";
    case AR_READ_PREFETCH:                  return "READ_PREFETCH";
    case AR_SYNC_EVENT:                     return "SYNC_EVENT";
    case AR_TIMER_EVENT:                    return "TIMER_EVENT";
    }
    return "?";
}

PRIVATE LPSTR map_thread_pri(DWORD dwPriority) {
    switch (dwPriority) {
    case THREAD_PRIORITY_ABOVE_NORMAL:
        return "ABOVE";

    case THREAD_PRIORITY_BELOW_NORMAL:
        return "BELOW";

    case THREAD_PRIORITY_HIGHEST:
        return "HIGHEST";

    case THREAD_PRIORITY_IDLE:
        return "IDLE";

    case THREAD_PRIORITY_LOWEST:
        return "LOWEST";

    case THREAD_PRIORITY_NORMAL:
        return "NORMAL";

    case THREAD_PRIORITY_TIME_CRITICAL:
        return "TIMECRIT";
    }
    return "?";
}

PRIVATE LPSTR map_length(DWORD dwLength, LPSTR lpBuf) {
    wsprintf(lpBuf, "%d", dwLength);
    return lpBuf;
}

#endif // defined(USE_PERF_DIAG)

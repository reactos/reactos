/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    perfdiag.hxx

Abstract:

    Performance diagnostics

Author:

    Richard L Firth (rfirth) 24-Jan-1997

Revision History:

    24-Jan-1997 rfirth
        Created

--*/

#if !defined(_PERFDIAG_)
#define _PERFDIAG_

//
// perf event (PE_) definitions
//

//#define USE_ICECAP_FOR_DOWNLOAD_PROFILING 1

#define PE_START                    1
#define PE_END                      2
#define PE_CLIENT_REQUEST_START     3
#define PE_CLIENT_REQUEST_END       4
#define PE_CLIENT_REQUEST_QUEUED    5
#define PE_WORKER_REQUEST_START     6
#define PE_WORKER_REQUEST_END       7
#define PE_APP_CALLBACK_START       8
#define PE_APP_CALLBACK_END         9
#define PE_NAMERES_START            10
#define PE_NAMERES_END              11
#define PE_CONNECT_START            12
#define PE_CONNECT_END              13
#define PE_SEND_START               14
#define PE_SEND_END                 15
#define PE_RECEIVE_START            16
#define PE_RECEIVE_END              17
#define PE_PEEK_RECEIVE_START       18
#define PE_PEEK_RECEIVE_END         19
#define PE_SOCKET_CLOSE_START       20
#define PE_SOCKET_CLOSE_END         21
#define PE_ACQUIRE_KEEP_ALIVE       22
#define PE_RELEASE_KEEP_ALIVE       23
#define PE_SOCKET_ERROR             24
#define PE_CACHE_READ_CHECK_START   25
#define PE_CACHE_READ_CHECK_END     26
#define PE_CACHE_WRITE_CHECK_START  27
#define PE_CACHE_WRITE_CHECK_END    28
#define PE_CACHE_RETRIEVE_START     29
#define PE_CACHE_RETRIEVE_END       30
#define PE_CACHE_READ_START         31
#define PE_CACHE_READ_END           32
#define PE_CACHE_WRITE_START        33
#define PE_CACHE_WRITE_END          34
#define PE_CACHE_CREATE_FILE_START  35
#define PE_CACHE_CREATE_FILE_END    36
#define PE_CACHE_CLOSE_FILE_START   37
#define PE_CACHE_CLOSE_FILE_END     38
#define PE_CACHE_EXPIRY_CHECK_START 39
#define PE_CACHE_EXPIRY_CHECK_END   40
#define PE_YIELD_SELECT_START       41
#define PE_YIELD_SELECT_END         42
#define PE_YIELD_OBJECT_WAIT_START  43
#define PE_YIELD_OBJECT_WAIT_END    44
#define PE_YIELD_SLEEP_START        45
#define PE_YIELD_SLEEP_END          46
#define PE_TRACE                    47
#define PE_ENTER_PATH               48
#define PE_LEAVE_PATH               49
#define PE_TRACE_PATH               50

//
// API prototypes
//

#if defined(__cplusplus)
extern "C" {
#endif

VOID
#if defined(_WINX32_)
__declspec(dllexport)
#else
__declspec(dllimport)
#endif
WINAPI
WininetPerfLog(
    IN DWORD dwEvent,
    IN DWORD dwInfo1,
    IN DWORD dwInfo2,
    IN HINTERNET hInternet
    );

#if defined(__cplusplus)
}
#endif

#if defined(_WINX32_)
#if defined(USE_PERF_DIAG)

//
// manifests
//

//#define DEFAULT_PE_LOG_RECORDS      4096
#define DEFAULT_PE_LOG_RECORDS      16384

//
// types
//

typedef struct {
    LARGE_INTEGER liTime;
    DWORD dwThreadId;
    DWORD dwThreadPriority;
    HINTERNET hInternet;
    DWORD dwEvent;
    DWORD dwInfo;
    DWORD dwInfo2;
} PERF_INFO, * LPPERF_INFO;

//
// prototypes
//

VOID
PerfSleep(
    DWORD dwMilliseconds
    );

int PerfSelect(
    int nfds,	
    fd_set FAR * readfds,	
    fd_set FAR * writefds,	
    fd_set FAR * exceptfds,	
    const struct timeval FAR * timeout 	
    );

DWORD
PerfWaitForSingleObject(
    HANDLE hObject,
    DWORD dwTimeout
    );	

//
// classes
//

class CPerfDiag {

private:

    LPBYTE m_lpbPerfBuffer;
    DWORD m_dwPerfBufferLen;
    LPBYTE m_lpbEnd;
    LPBYTE m_lpbNext;
    BOOL m_bFull;
    BOOL m_bStarted;
    BOOL m_bStartFinished;
    LARGE_INTEGER m_liStartTime;

    LPPERF_INFO get_next_record(VOID);

    VOID get_perf_time(PLARGE_INTEGER pTime) {
        QueryPerformanceCounter(pTime);
    }

    VOID get_time(LPPERF_INFO lpInfo) {
        get_perf_time(&lpInfo->liTime);
    }

    VOID free_perf_buffer(VOID) {
        if (m_lpbPerfBuffer) {
            m_lpbPerfBuffer = (LPBYTE)FREE_MEMORY(m_lpbPerfBuffer);

            INET_ASSERT(!m_lpbPerfBuffer);

        }
    }

    VOID perf_start(VOID) {
        while (!m_bStartFinished) {
            if (InterlockedExchange((LPLONG)&m_bStarted, TRUE) == FALSE) {
                get_perf_time(&m_liStartTime);
                Init();
                m_bStartFinished = TRUE;
            }
            if (!m_bStartFinished) {
                Sleep(0);
            }
        }
    }

public:

    CPerfDiag();
    ~CPerfDiag();

    VOID Init(DWORD dwNumberOfRecords = DEFAULT_PE_LOG_RECORDS) {
        CreateBuffer(dwNumberOfRecords);
    }

    VOID CreateBuffer(LPBYTE lpbBuffer, DWORD dwBufferLength) {
    }

    VOID CreateBuffer(DWORD dwNumberOfRecords = DEFAULT_PE_LOG_RECORDS) {
        free_perf_buffer();
        m_dwPerfBufferLen = dwNumberOfRecords * sizeof(PERF_INFO);
        m_lpbPerfBuffer = (LPBYTE)ALLOCATE_MEMORY(LMEM_FIXED, m_dwPerfBufferLen);
        if (m_lpbPerfBuffer) {
            m_lpbEnd = m_lpbPerfBuffer + m_dwPerfBufferLen;
            m_lpbNext = m_lpbPerfBuffer;
            m_bFull = FALSE;
        } else {
            OutputDebugString("*** Cannot create performance buffer\n");
        }
    }

    VOID Log(DWORD dwEvent, DWORD dwInfo = 0);
    VOID Log(DWORD dwEvent, DWORD dwInfo, DWORD dwThreadId, HINTERNET hInternet);
    VOID Log(DWORD dwEvent, DWORD dwInfo, DWORD dwInfo2, DWORD dwThreadId, HINTERNET hInternet);
    VOID Dump(VOID);
};

//
// global data
//

extern CPerfDiag * GlobalPerfDiag;

//
// macros
//

#define PERF_INIT() \
    if (!GlobalPerfDiag) GlobalPerfDiag = new CPerfDiag

#define PERF_END() \
    if (GlobalPerfDiag) delete GlobalPerfDiag

#define PERF_LOG \
    if (GlobalPerfDiag) GlobalPerfDiag->Log

#define PERF_DUMP() \
    if (GlobalPerfDiag) GlobalPerfDiag->Dump()

#define PERF_Sleep(n) \
    PerfSleep(n)

#define PERF_Select(n, pReadfds, pWritefds, pExceptfds, pTimeout) \
    PerfSelect(n, pReadfds, pWritefds, pExceptfds, pTimeout)

#define PERF_WaitForSingleObject(hObject, dwTimeout) \
    PerfWaitForSingleObject(hObject, dwTimeout)

#else

#define PERF_INIT() \
    /* NOTHING */

#define PERF_END() \
    /* NOTHING */

#define PERF_LOG \
    (VOID)

#define PERF_DUMP() \
    /* NOTHING */

#define PERF_Sleep(n) \
    Sleep(n)

#define PERF_Select(n, pReadfds, pWritefds, pExceptfds, pTimeout) \
    _I_select(n, pReadfds, pWritefds, pExceptfds, pTimeout)

#define PERF_WaitForSingleObject(hObject, dwTimeout) \
    WaitForSingleObject(hObject, dwTimeout)

#endif // defined(USE_PERF_DIAG)
#endif // defined(_WINX32_)

//#define PERF_ENTER(Name) \
//    StartCAP()

//#define PERF_LEAVE(Name) \
//    StopCAP()


#if defined(USE_PERF_DIAG)

#define PERF_LOG_EX \
    WininetPerfLog

#define PERF_ID(Name) \
    (LPCSTR)(# Name)

#define PERF_TRACE(Name, Info) \
    WininetPerfLog(PE_TRACE_PATH, Info, (DWORD)PERF_ID(Name), 0)

#define PERF_ENTER(Name) \
    WininetPerfLog(PE_ENTER_PATH, 0, (DWORD)PERF_ID(Name), 0)

#define PERF_LEAVE(Name) \
    WininetPerfLog(PE_LEAVE_PATH, 0, (DWORD)PERF_ID(Name), 0)

#else

#define PERF_LOG_EX \
    /* NOTHING */

#define PERF_ID(Name) \
    /* NOTHING */

#define PERF_TRACE(Name, Info) \
    /* NOTHING */


#ifdef USE_ICECAP_FOR_DOWNLOAD_PROFILING

#define PERF_ENTER(Name) \
    StartCAP()

#define PERF_LEAVE(Name) \
    StopCAP()

#else

#define PERF_ENTER(Name) \
    /* NOTHING */

#define PERF_LEAVE(Name) \
    /* NOTHING */

#endif // defined(USE_ICECAP_FOR_DOWNLOAD_PROFILING)


#endif // defined(USE_PERF_DIAG)


#define STOP_SENDREQ_PERF() \
    /* nothing */

#define START_SENDREQ_PERF() \
    /* nothing */

#endif // defined(_PERFDIAG_)

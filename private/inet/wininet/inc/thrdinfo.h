/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    thrdinfo.h

Abstract:

    Per-thread structure definitions/macros

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

#define INTERNET_THREAD_INFO_SIGNATURE  'drhT'  // "Thrd"

//
// forward references
//

class CFsm;

//
// types
//

//
// INTERNET_THREAD_INFO - per-thread information, handily referenced via a TLS
// slot
//

typedef struct _INTERNET_THREAD_INFO {

    //
    // List - doubly linked list that we delete on DLL_PROCESS_DETACH
    //

    LIST_ENTRY List;

#if INET_DEBUG

    //
    // Signature - lets us know that this structure is probably an
    // INTERNET_THREAD_INFO
    //

    DWORD Signature;

#endif

    //
    // ThreadId - used to identify this thread within a process
    //

    DWORD ThreadId;

    //
    // ErrorNumber - arbitrary error code, supplied in InternetSetLastError
    //

    DWORD ErrorNumber;

    //
    // hErrorText - we store the last error text on a per-thread basis. This
    // handle identifies a moveable buffer
    //

    HLOCAL hErrorText;

    //
    // ErrorTextLength - length of the error text in hErrorText
    //

    DWORD ErrorTextLength;

    //
    // Context - arbitrary app-supplied context value. This is used by
    // StatusCallback to identify which operation the callback is for. We keep
    // the context in the per-thread information so we don't have to pass it
    // around, nor across the wire in the case of an RPC callback
    //

    DWORD_PTR Context;

    //
    // hObject - the current Internet object handle being used in this API. We
    // need this to maintain context e.g. when we want to get timeout values
    //

    HINTERNET hObject;

    //
    // hObjectMapped - this is the address of the real object mapped to hObject
    //

    HINTERNET hObjectMapped;

    //
    // IsAsyncWorkerThread - TRUE if this thread is an async worker thread
    //

    BOOL IsAsyncWorkerThread;

    //
    // InCallback - TRUE if we have made an app callback. Used to detect
    // re-entrancy
    //

    BOOL InCallback;

    //
    // IsAutoProxyProxyThread - TRUE if we are the thread running auto-proxy requests.  Used
    //  to allow direct shutdown of auto-proxy during PROCESS_DETACH
    //

    BOOL IsAutoProxyProxyThread;

    //
    // NestedRequests - incremented when we detect that we're processing an API
    // in the async worker thread context. If this API then calls other APIs,
    // then we need to treat (mapped) handles differently in the called APIs
    //

    DWORD NestedRequests;

    //
    // dwMappedErrorCode - the real error code returned by e.g. a winsock API,
    // before it was mapped to a WinInet error
    //

    DWORD dwMappedErrorCode;

    //
    // Fsm - currently executing Finite State Machine
    //

    CFsm * Fsm;

#ifdef ENABLE_DEBUG

    //
    // IsAsyncSchedulerThread - TRUE if this INTERNET_THREAD_INFO belongs to the
    // one-and-only async scheduler thread
    //

    BOOL IsAsyncSchedulerThread;

    //
    // per-thread debug variables
    //

    //
    // Pointer to LIFO (stack) of INTERNET_DEBUG_RECORDs. Used to generate
    // indented call-tracing for diagnostics
    //

    LPINTERNET_DEBUG_RECORD Stack;

    //
    // CallDepth - nesting level for calls
    //

    int CallDepth;

    //
    // IndentIncrement - the current indent level. Number of spaces
    //

    int IndentIncrement;

    //
    // StartTime and StopTime - used for timing calls to e.g. send(), recv()
    //

    DWORD StartTime;
    DWORD StopTime;

    DWORD MajorCategoryFlags;
    DWORD MinorCategoryFlags;

#endif // #ifdef ENABLE_DEBUG

} INTERNET_THREAD_INFO, *LPINTERNET_THREAD_INFO;

//
// macros
//

//
// InternetClearLastError - frees the response text buffer for this thread
//

#define InternetClearLastError() \
    InternetSetLastError(0, NULL, 0, 0)

//
// InternetResetContext - resets the per-thread call-back context value
//

#define InternetResetContext() \
    InternetSetContext(INTERNET_NO_CALLBACK)

//
// InternetResetObjectHandle - resets the per-thread current object handle
//

#define InternetResetObjectHandle() \
    InternetSetObjectHandle(NULL)

//
// _InternetIncNestingCount - increments nesting level count
//

#define _InternetIncNestingCount() \
    lpThreadInfo->NestedRequests++;

// ** debug version
//#define _InternetIncNestingCount() \
//    if ( lpThreadInfo->NestedRequests > 0xffff ) { \
//        OutputDebugString("InternetIncNestingCount, inc over threshold, contact arthurbi, x68073 (sechs)\n"); \
//        DebugBreak(); \
//    } \
//    lpThreadInfo->NestedRequests++;

//
// _InternetDecNestingCount - decrements nesting level count
//

#define _InternetDecNestingCount(dwNestingLevel) \
    lpThreadInfo->NestedRequests -= dwNestingLevel;

// ** debug version
//#define _InternetDecNestingCount(dwNestingLevel) \
//    if ( lpThreadInfo->NestedRequests == 0 ) { \
//        OutputDebugString("InternetDecNestingCount, attempting to dec 0, contact arthurbi, x68073 (sieben)\n"); \
//        DebugBreak(); \
//    } \
//    if ( dwNestingLevel != 1 && dwNestingLevel != 0 ) { \
//        OutputDebugString("InternetDecNestingCount, invalid nesting level, contact arthurbi, x68073 (acht)\n"); \
//        DebugBreak(); \
//    } \
//    lpThreadInfo->NestedRequests -= dwNestingLevel;

//
// _InternetSetObjectHandle - set the object handle given the thread info block
//

#define _InternetSetObjectHandle(lpThreadInfo, hInternet, hMapped) \
    DEBUG_PRINT(HTTP,   \
            INFO,       \
            ("Setting new obj handle on thrd=%x, old=%x, new=%x (map: old=%x, new=%x)\n", \
            lpThreadInfo, \
            lpThreadInfo->hObject, \
            hInternet, \
            lpThreadInfo->hObjectMapped, \
            hMapped \
            )); \
    if ( lpThreadInfo->IsAutoProxyProxyThread ) \
        GlobalProxyInfo.SetAbortHandle(hInternet); \
    lpThreadInfo->hObject = hInternet; \
    lpThreadInfo->hObjectMapped = hMapped;

//
// _InternetSetContext - set the object context given the thread info block
//

#define _InternetSetContext(lpThreadInfo, dwContext) \
    DEBUG_PRINT(HTTP,   \
            INFO,       \
            ("Setting new context on thrd=%x, old=%x, new=%x\n", \
            lpThreadInfo, \
            lpThreadInfo->Context, \
            dwContext \
            )); \
    lpThreadInfo->Context = dwContext;

//
// _InternetClearLastError - clear the last error info given the thread info
// block
//

#define _InternetClearLastError(lpThreadInfo) \
    _InternetSetLastError(lpThreadInfo, 0, NULL, 0, 0)

//
// _InternetResetObjectHandle - clear the object handle given the thread info
// block
//

#define _InternetResetObjectHandle(lpThreadInfo) \
    _InternetSetObjectHandle(lpThreadInfo, NULL, NULL)

//
// _InternetGetObjectHandle - retrieves the object handle from the per-thread
// info block
//

#define _InternetGetObjectHandle(lpThreadInfo) \
    lpThreadInfo->hObject

//
// _InternetGetMappedObjectHandle - retrieves the mapped object handle from the
// per-thread info block
//

#define _InternetGetMappedObjectHandle(lpThreadInfo) \
    lpThreadInfo->hObjectMapped

//
// _InternetGetContext - retrieve the context from the per-thread info block
//

#define _InternetGetContext(lpThreadInfo) \
    lpThreadInfo->Context

//
// _InternetResetContext - reset context in per-thread info block given
// per-thread info block
//

#define _InternetResetContext(lpThreadInfo) \
    _InternetSetContext(lpThreadInfo, 0)

//
// InternetDisableAsync - turns off the async worker thread indication in the
// thread info block
//

#define _InternetDisableAsync(lpThreadInfo) \
    _InternetSetAsync(FALSE)

//
// InternetEnableAsync - turns off the async worker thread indication in the
// thread info block
//

#define _InternetEnableAsync(lpThreadInfo, Val) \
    _InternetSetAsync(TRUE)

//
// _InternetGetAsync - returns the async worker thread indication from the
// thread info block
//

#define _InternetGetAsync(lpThreadInfo) \
    lpThreadInfo->IsAsyncWorkerThread

//
// _InternetSetAsync - turns on or off the async worker thread indication in the
// thread info block
//

#define _InternetSetAsync(lpThreadInfo, Val) \
    lpThreadInfo->IsAsyncWorkerThread = Val

#define _InternetGetInCallback(lpThreadInfo) \
    lpThreadInfo->InCallback

#define _InternetSetInCallback(lpThreadInfo) \
    lpThreadInfo->InCallback = TRUE

#define _InternetResetInCallback(lpThreadInfo) \
    lpThreadInfo->InCallback = FALSE

#define _InternetSetAutoProxy(lpThreadInfo) \
    lpThreadInfo->IsAutoProxyProxyThread = TRUE

#if INET_DEBUG

#define CHECK_INTERNET_THREAD_INFO(lpThreadInfo) \
    INET_ASSERT(lpThreadInfo->Signature == INTERNET_THREAD_INFO_SIGNATURE)

#else

#define CHECK_INTERNET_THREAD_INFO(lpThreadInfo) \
    /* NOTHING */

#endif

//
// prototypes
//

#define UI_ACTION_CODE_NONE_TAKEN                   0
#define UI_ACTION_CODE_BLOCKED_FOR_INTERNET_HANDLE  1
#define UI_ACTION_CODE_BLOCKED_FOR_USER_INPUT       2
#define UI_ACTION_CODE_USER_ACTION_COMPLETED        3


DWORD
ChangeUIBlockingState(
    IN HINTERNET hRequestMapped,
    IN DWORD     dwError,
    OUT LPDWORD  lpdwActionTaken,
    OUT LPDWORD  lpdwResultCode,
    IN OUT LPVOID * lplpResultData
    );

DWORD
ResumeAfterUserInput(
    IN HINTERNET hRequestMapped,
    IN DWORD     dwResultCode,
    OUT LPBOOL   pfItemResumed
    );

LPINTERNET_THREAD_INFO
InternetCreateThreadInfo(
    IN BOOL SetTls
    );

VOID
InternetDestroyThreadInfo(
    VOID
    );

VOID
InternetFreeThreadInfo(
    IN LPINTERNET_THREAD_INFO lpThreadInfo
    );

VOID
InternetTerminateThreadInfo(
    VOID
    );

LPINTERNET_THREAD_INFO
InternetGetThreadInfo(
    VOID
    );

VOID
InternetSetThreadInfo(
    IN LPINTERNET_THREAD_INFO lpThreadInfo
    );

DWORD
InternetIndicateStatusAddress(
    IN DWORD dwInternetStatus,
    IN LPSOCKADDR lpSockAddr,
    IN DWORD dwSockAddrLength
    );

DWORD
InternetIndicateStatusString(
    IN DWORD dwInternetStatus,
    IN LPSTR lpszStatusInfo
    );

DWORD
InternetIndicateStatus(
    IN DWORD dwInternetStatus,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

DWORD
InternetIndicateStatusNewHandle(
    IN LPVOID hInternetMapped
    );

DWORD
InternetSetLastError(
    IN DWORD ErrorNumber,
    IN LPSTR ErrorText,
    IN DWORD ErrorTextLength,
    IN DWORD Flags
    );

#define SLE_APPEND          0x00000001
#define SLE_ZERO_TERMINATE  0x00000002

DWORD
_InternetSetLastError(
    IN LPINTERNET_THREAD_INFO lpThreadInfo,
    IN DWORD ErrorNumber,
    IN LPSTR ErrorText,
    IN DWORD ErrorTextLength,
    IN DWORD Flags
    );

LPSTR
InternetLockErrorText(
    VOID
    );

VOID
InternetUnlockErrorText(
    VOID
    );

VOID
InternetSetContext(
    IN DWORD_PTR dwContext
    );

VOID
InternetSetObjectHandle(
    IN HINTERNET hInternet,
    IN HINTERNET hInternetMapped
    );

HINTERNET
InternetGetObjectHandle(
    VOID
    );

HINTERNET
InternetGetMappedObjectHandle(
    VOID
    );

//
// external data
//

extern SERIALIZED_LIST ThreadInfoList;

#if defined(__cplusplus)
}
#endif

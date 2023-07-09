#ifndef _HTTPFILT_
#define _HTTPFILT_

//#include <wininet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SZFN_FILTEROPEN "HttpFilterOpen"

BOOL
WINAPI
HttpFilterOpen
(
    OUT LPVOID *lppvFilterContext,
    IN  LPCSTR szFilterName,
    IN  LPVOID lpReserved
);

typedef BOOL (WINAPI *PFN_FILTEROPEN)
   (LPVOID*, LPCSTR, LPVOID);

#define SZFN_FILTERCLOSE "HttpFilterClose"

BOOL
WINAPI
HttpFilterClose
(
    IN     LPVOID     lpvFilterContext,  // context created by HttpFilterOpen
    IN     BOOL       fInShutdown        // TRUE if in DLL_PROCESS_DETACH
);

typedef BOOL (WINAPI *PFN_FILTERCLOSE)
   (LPVOID, BOOL);

// Per Transaction
// There are called one for every HTTP transaction that WinInet performs.

#define SZFN_FILTERBEGINNINGTRANSACTION "HttpFilterBeginningTransaction"

BOOL
WINAPI
HttpFilterBeginningTransaction
(
    IN     LPVOID     lpvFilterContext,  // context created by HttpFilterOpen
    IN OUT LPVOID*    lppvTransactionContext,
    IN     HINTERNET  hRequest,
    IN     LPVOID     lpReserved
);

typedef BOOL (WINAPI *PFN_FILTERBEGINNINGTRANSACTION)
   (LPVOID, LPVOID*, HINTERNET, LPVOID);

//This is called when a transaction begins.  It gives the caller an oppurtunity
// to examine the request header and modify it.

#define SZFN_FILTERONRESPONSE "HttpFilterOnResponse"

BOOL
WINAPI
HttpFilterOnResponse
(
    IN     LPVOID     lpvFilterContext,  // context created by HttpFilterOpen
    IN OUT LPVOID*    lppvTransactionContext,
    IN     HINTERNET  hRequest,
    IN     LPVOID     lpReserved
);

typedef BOOL (WINAPI *PFN_FILTERONRESPONSE)
   (LPVOID, LPVOID*, HINTERNET, LPVOID);

// This is called when the HTTP response returns and all of the HTTP headers are
// vailable to examine.

#define SZFN_FILTERONBLOCKINGOPS "HttpFilterOnBlockingOps"

BOOL
WINAPI
HttpFilterOnBlockingOps
(
    IN     LPVOID     lpvFilterContext,  // context created by HttpFilterOpen
    IN OUT LPVOID*    lppvTransactionContext,
    IN     HINTERNET  hRequest,
    IN     HWND       hWnd,
    IN     LPVOID     lpReserved
);

typedef BOOL (WINAPI *PFN_FILTERONBLOCKINGOPS)
        (LPVOID, LPVOID*, HINTERNET, HWND, LPVOID);

// Called in response to any of the above APIs returning FALSE and setting the
// GetLastError() value to INTERNET_ERROR_NEED_BLOCKING_UI.  The caller can put
// up UI in this situation.using hWnd as the parent Window.

#define SZFN_FILTERONTRANSACTIONCOMPLETE "HttpFilterOnTransactionComplete"

BOOL
WINAPI
HttpFilterOnTransactionComplete
(
    IN     LPVOID     lpvFilterContext,  // context created by HttpFilterOpen
    IN OUT LPVOID*    lppvTransactionContext,
    IN     HINTERNET  hRequest,
    IN     LPVOID     lpReserved
);

typedef BOOL (WINAPI *PFN_FILTERONTRANSACTIONCOMPLETE)
   (LPVOID, LPVOID*, HINTERNET, LPVOID);

// Called when I the transaction is complete.  This gives the caller an
// opportunity to clean up any transaction specific data.  Filter returns TRUE
// to indicate "I took no action - you should proceed", FALSE to indicate that
// the value from GetLastError() will have ben set.


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _HTTPFILT_

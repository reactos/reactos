#ifndef ASYNC_INET
#define ASYNC_INET


enum ASYNC_EVENT
{
    ASYNCINET_DATA,           // wParam is the Data retrieved from the internet, lParam is the length of Data

    ASYNCINET_COMPLETE,       // wParam and lParam are not used.
                              // when receiving this, AsyncInet will be free soon and should not used anymore

    ASYNCINET_CANCELLED,      // wParam and lParam are not used.
                              // when receiving this, AsyncInet will be free soon and should not used anymore

    ASYNCINET_ERROR           // wParam is not used. lParam specify the error code (if there is one).
                              // when receiving this, AsyncInet will be free soon and should not used anymore
};

typedef struct __AsyncInet ASYNCINET, * pASYNCINET;

typedef int
(*ASYNCINET_CALLBACK)(
    pASYNCINET AsyncInet,
    ASYNC_EVENT Event,
    WPARAM wParam,
    LPARAM lParam,
    VOID* Extension
    );

typedef struct __AsyncInet
{
    HINTERNET hInternet;
    HINTERNET hInetFile;

    HANDLE hEventHandleCreated;

    UINT ReferenceCnt;
    CRITICAL_SECTION CriticalSection;
    HANDLE hEventHandleClose;

    BOOL bIsOpenUrlComplete;

    BOOL bCancelled;

    BYTE ReadBuffer[4096];
    DWORD BytesRead;

    ASYNCINET_CALLBACK Callback;
    VOID* Extension;
} ASYNCINET, * pASYNCINET;

pASYNCINET AsyncInetDownload(LPCWSTR lpszAgent,
    DWORD   dwAccessType,
    LPCWSTR lpszProxy,
    LPCWSTR lpszProxyBypass,
    LPCWSTR lpszUrl,
    BOOL bAllowCache,
    ASYNCINET_CALLBACK Callback,
    VOID* Extension
    );

BOOL AsyncInetCancel(pASYNCINET AsyncInet);

VOID AsyncInetRelease(pASYNCINET AsyncInet);

#endif

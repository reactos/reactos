#ifndef ASYNC_INET
#define ASYNC_INET

#include <windows.h>
#include <wininet.h>

enum ASYNC_EVENT
{
    ASYNCINET_DATA,           // wParam is the Data retrieved from the internet, lParam is the length of Data
    ASYNCINET_COMPLETE,       // wParam and lParam is not used
    ASYNCINET_CANCELLED,      // wParam and lParam is not used
    ASYNCINET_ERROR           // wParam is not used. lParam specify the error code (if there is one)
};

typedef struct __AsyncInet ASYNCINET, * pASYNCINET;

typedef int
(* ASYNCINET_CALLBACK)(
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

    long long HandleClosedCnt;

    BOOL bIsOpenUrlComplete;

    BOOL bIsCancelled;

    BYTE ReadBuffer[4096];
    DWORD BytesRead;

    ASYNCINET_CALLBACK Callback;
    VOID* Extension;
} ASYNCINET, * pASYNCINET;

pASYNCINET AsyncInetDownloadW(LPCWSTR lpszAgent,
    DWORD   dwAccessType,
    LPCWSTR lpszProxy,
    LPCWSTR lpszProxyBypass,
    LPCWSTR lpszUrl,
    BOOL bAllowCache,
    ASYNCINET_CALLBACK Callback,
    VOID* Extension
    );

BOOL AsyncInetCancel(pASYNCINET AsyncInet);

#endif

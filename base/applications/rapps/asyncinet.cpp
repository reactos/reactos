/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Async Internet operation using WinINet
 * COPYRIGHT:   Copyright 2020 He Yang            (1160386205@qq.com)
 */

#include "rapps.h"
#include <wininet.h>
#include <atlbase.h>
#include "asyncinet.h"


BOOL AsyncInetIsCanceled(pASYNCINET AsyncInet);
BOOL AsyncInetAcquire(pASYNCINET AsyncInet);
VOID AsyncInetRelease(pASYNCINET AsyncInet);
int AsyncInetPerformCallback(pASYNCINET AsyncInet,
    ASYNC_EVENT Event,
    WPARAM wParam,
    LPARAM lParam
    );
VOID CALLBACK AsyncInetStatusCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    );
VOID AsyncInetReadFileLoop(pASYNCINET AsyncInet);
BOOL AsyncInetCleanUp(pASYNCINET AsyncInet);
VOID AsyncInetFree(pASYNCINET AsyncInet);

pASYNCINET AsyncInetDownload(LPCWSTR lpszAgent,
    DWORD   dwAccessType,
    LPCWSTR lpszProxy,
    LPCWSTR lpszProxyBypass,
    LPCWSTR lpszUrl,
    BOOL bAllowCache,
    ASYNCINET_CALLBACK Callback,
    VOID* Extension
    ) // allocate memory for AsyncInet and start a download task
{
    pASYNCINET AsyncInet = NULL;
    BOOL bSuccess = FALSE;
    DWORD InetOpenUrlFlag = 0;
    INTERNET_STATUS_CALLBACK OldCallbackFunc;

    AsyncInet = (pASYNCINET)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ASYNCINET));
    if (!AsyncInet)
    {
        OutputDebugStringA("At File: " __FILE__ " HeapAlloc returned 0\n");
        return 0;
    }

    AsyncInet->bCancelled = FALSE;

    AsyncInet->hInternet = NULL;
    AsyncInet->hInetFile = NULL;

    AsyncInet->Callback = Callback;
    AsyncInet->Extension = Extension;

    AsyncInet->hEventHandleCreated = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection(&(AsyncInet->CriticalSection));
    AsyncInet->ReferenceCnt = 1; // 1 for callee itself
    AsyncInet->hEventHandleClose = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (AsyncInet->hEventHandleCreated && AsyncInet->hEventHandleClose)
    {
        AsyncInet->hInternet = InternetOpenW(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, INTERNET_FLAG_ASYNC);

        if (AsyncInet->hInternet)
        {
            OldCallbackFunc = InternetSetStatusCallbackW(AsyncInet->hInternet, AsyncInetStatusCallback);
            if (OldCallbackFunc != INTERNET_INVALID_STATUS_CALLBACK)
            {
                InetOpenUrlFlag |= INTERNET_FLAG_PASSIVE | INTERNET_FLAG_RESYNCHRONIZE;
                if (!bAllowCache)
                {
                    InetOpenUrlFlag |= INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD;
                }

                AsyncInet->hInetFile = InternetOpenUrlW(AsyncInet->hInternet, lpszUrl, 0, 0, InetOpenUrlFlag, (DWORD_PTR)AsyncInet);

                if (AsyncInet->hInetFile)
                {
                    // operate complete synchronously
                    bSuccess = TRUE;
                    AsyncInetReadFileLoop(AsyncInet);
                }
                else
                {
                    if (GetLastError() == ERROR_IO_PENDING)
                    {
                        // everything fine. waiting for handle created
                        switch (WaitForSingleObject(AsyncInet->hEventHandleCreated, INFINITE))
                        {
                        case WAIT_OBJECT_0:
                            if (AsyncInet->hInetFile)
                            {
                                bSuccess = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!bSuccess)
    {
        AsyncInetFree(AsyncInet);
        AsyncInet = NULL;
    }

    if (AsyncInet)
    {
        // add reference count for caller.
        // the caller is responsible for call AsyncInetRelease when no longer using it.
        AsyncInetAcquire(AsyncInet);
    }
    return AsyncInet;
}

BOOL AsyncInetCancel(pASYNCINET AsyncInet) // mark as cancelled (this will send a cancel notificaion at last) and do graceful cleanup.
{
    if (AsyncInet)
    {
        HINTERNET hInetFile;
        EnterCriticalSection(&(AsyncInet->CriticalSection));
        AsyncInet->bCancelled = TRUE;
        hInetFile = AsyncInet->hInetFile;
        AsyncInet->hInetFile = NULL;
        LeaveCriticalSection(&(AsyncInet->CriticalSection));

        if (hInetFile)
        {
            InternetCloseHandle(hInetFile);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL AsyncInetIsCanceled(pASYNCINET AsyncInet) // if returned TRUE, no operation should be exectued further
{
    if (AsyncInet)
    {
        EnterCriticalSection(&(AsyncInet->CriticalSection));
        if (AsyncInet->bCancelled)
        {
            LeaveCriticalSection(&(AsyncInet->CriticalSection));
            return TRUE;
        }
        LeaveCriticalSection(&(AsyncInet->CriticalSection));
    }
    return FALSE;
}

BOOL AsyncInetAcquire(pASYNCINET AsyncInet) // try to increase refcnt by 1. if returned FALSE, AsyncInet should not be used anymore
{
    BOOL bResult = FALSE;
    if (AsyncInet)
    {
        EnterCriticalSection(&(AsyncInet->CriticalSection));
        ATLASSERT(AsyncInet->ReferenceCnt > 0);
        if (!AsyncInetIsCanceled(AsyncInet))
        {
            AsyncInet->ReferenceCnt++;
            bResult = TRUE;
        }
        // otherwise (AsyncInet->bCleanUp == TRUE)
        // AsyncInetAcquire will return FALSE.
        // In this case, any thread should no longer use this AsyncInet

        LeaveCriticalSection(&(AsyncInet->CriticalSection));
    }

    return bResult;
}

VOID AsyncInetRelease(pASYNCINET AsyncInet) // try to decrease refcnt by 1
{
    BOOL bCleanUp = FALSE;
    if (AsyncInet)
    {
        EnterCriticalSection(&(AsyncInet->CriticalSection));

        ATLASSERT(AsyncInet->ReferenceCnt);
        AsyncInet->ReferenceCnt--;
        if (AsyncInet->ReferenceCnt == 0)
        {
            bCleanUp = TRUE;
        }

        LeaveCriticalSection(&(AsyncInet->CriticalSection));

        if (bCleanUp)
        {
            AsyncInetCleanUp(AsyncInet);
        }
    }
}

int AsyncInetPerformCallback(pASYNCINET AsyncInet,
    ASYNC_EVENT Event,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (AsyncInet && AsyncInet->Callback)
    {
        return AsyncInet->Callback(AsyncInet, Event, wParam, lParam, AsyncInet->Extension);
    }
    return 0;
}

VOID CALLBACK AsyncInetStatusCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    )
{
    pASYNCINET AsyncInet = (pASYNCINET)dwContext;
    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_HANDLE_CREATED:
    {
        // retrieve handle created by InternetOpenUrlW
        AsyncInet->hInetFile = *((LPHINTERNET)lpvStatusInformation);
        SetEvent(AsyncInet->hEventHandleCreated);
        break;
    }
    case INTERNET_STATUS_HANDLE_CLOSING:
    {
        if (AsyncInetIsCanceled(AsyncInet))
        {
            AsyncInetPerformCallback(AsyncInet, ASYNCINET_CANCELLED, 0, 0);
        }
        SetEvent(AsyncInet->hEventHandleClose);
        break;
    }
    case INTERNET_STATUS_REQUEST_COMPLETE:
    {
        INTERNET_ASYNC_RESULT* AsyncResult = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;

        if (!AsyncInet->hInetFile)
        {
            if (!AsyncInetIsCanceled(AsyncInet))
            {
                // some error occurs during InternetOpenUrl
                // and INTERNET_STATUS_HANDLE_CREATED is skipped
                SetEvent(AsyncInet->hEventHandleCreated);
                break;
            }
        }

        switch (AsyncResult->dwError)
        {
        case ERROR_SUCCESS:
        {
            // there are two possibilities:
            // 1: InternetOpenUrlW (async mode) complete. (this should be only for the first time)
            // 2: InternetReadFile (async mode) complete.
            // so I use bIsOpenUrlComplete field in ASYNCINET to indicate this.

            if (!(AsyncInet->bIsOpenUrlComplete)) // InternetOpenUrlW completed
            {
                AsyncInet->bIsOpenUrlComplete = TRUE;
            }
            else // asynchronous InternetReadFile complete
            {
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_DATA, (WPARAM)(AsyncInet->ReadBuffer), (LPARAM)(AsyncInet->BytesRead));
            }

            AsyncInetReadFileLoop(AsyncInet);
        }
        break;
        case ERROR_INVALID_HANDLE:
        case ERROR_INTERNET_OPERATION_CANCELLED:
        case ERROR_CANCELLED:
            if (AsyncInetIsCanceled(AsyncInet))
            {
                AsyncInetRelease(AsyncInet);
                break;
            }

            // fall down
        default:
            // something went wrong
            if (AsyncInetIsCanceled(AsyncInet))
            {
                // sending both ASYNCINET_ERROR and ASYNCINET_CANCELLED may lead to unpredictable behavior
                // TODO: log the error
                AsyncInetRelease(AsyncInet);
            }
            else
            {
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_ERROR, 0, (LPARAM)(AsyncResult->dwError));
                AsyncInetRelease(AsyncInet);
            }
            break;
        }
        break;
    }
    }
    return;
}

VOID AsyncInetReadFileLoop(pASYNCINET AsyncInet)
{
    if ((!AsyncInet) || (!AsyncInet->hInetFile))
    {
        return;
    }

    while (1)
    {
        if (AsyncInetIsCanceled(AsyncInet))
        {
            // abort now.
            AsyncInetRelease(AsyncInet);
            break;
        }

        BOOL bRet = InternetReadFile(AsyncInet->hInetFile,
            AsyncInet->ReadBuffer,
            _countof(AsyncInet->ReadBuffer),
            &(AsyncInet->BytesRead));

        if (bRet)
        {
            if (AsyncInet->BytesRead == 0)
            {
                // everything read. now complete.
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_COMPLETE, 0, 0);
                AsyncInetRelease(AsyncInet);
                break;
            }
            else
            {
                // read completed immediately.
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_DATA, (WPARAM)(AsyncInet->ReadBuffer), (LPARAM)(AsyncInet->BytesRead));
            }
        }
        else
        {
            DWORD dwError;
            if ((dwError = GetLastError()) == ERROR_IO_PENDING)
            {
                // performing asynchronous IO, everything OK.
                break;
            }
            else
            {
                if (dwError == ERROR_INVALID_HANDLE ||
                    dwError == ERROR_INTERNET_OPERATION_CANCELLED ||
                    dwError == ERROR_CANCELLED)
                {
                    if (AsyncInetIsCanceled(AsyncInet))
                    {
                        // not an error. just normally cancelling
                        AsyncInetRelease(AsyncInet);
                        break;
                    }
                }

                if (!AsyncInetIsCanceled(AsyncInet)) // can not send both ASYNCINET_ERROR and ASYNCINET_CANCELLED
                {
                    AsyncInetPerformCallback(AsyncInet, ASYNCINET_ERROR, 0, dwError);
                }
                else
                {
                    // TODO: log the error
                }
                AsyncInetRelease(AsyncInet);
                break;
            }
        }
    }
    return;
}

BOOL AsyncInetCleanUp(pASYNCINET AsyncInet) // close all handle and clean up
{
    if (AsyncInet)
    {
        ATLASSERT(AsyncInet->ReferenceCnt == 0);
        // close the handle, waiting for all pending request cancelled.

        if (AsyncInet->bCancelled) // already closed
        {
            AsyncInet->hInetFile = NULL;
        }
        else if (AsyncInet->hInetFile)
        {
            InternetCloseHandle(AsyncInet->hInetFile);
            AsyncInet->hInetFile = NULL;
        }

        // only cleanup when handle closed notification received
        switch (WaitForSingleObject(AsyncInet->hEventHandleClose, INFINITE))
        {
        case WAIT_OBJECT_0:
        {
            AsyncInetFree(AsyncInet); // now safe to free the structure
            return TRUE;
        }
        default:
            ATLASSERT(FALSE);
            AsyncInetFree(AsyncInet);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

VOID AsyncInetFree(pASYNCINET AsyncInet) // close all handles, free the memory occupied by AsyncInet
{
    if (AsyncInet)
    {
        DeleteCriticalSection(&(AsyncInet->CriticalSection));

        if (AsyncInet->hEventHandleCreated)
        {
            CloseHandle(AsyncInet->hEventHandleCreated);
            AsyncInet->hEventHandleCreated = NULL;
        }
        if (AsyncInet->hEventHandleClose)
        {
            CloseHandle(AsyncInet->hEventHandleClose);
            AsyncInet->hEventHandleClose = NULL;
        }
        if (AsyncInet->hInternet)
        {
            InternetCloseHandle(AsyncInet->hInternet);
            AsyncInet->hInternet = NULL;
        }
        if (AsyncInet->hInetFile)
        {
            InternetCloseHandle(AsyncInet->hInetFile);
            AsyncInet->hInetFile = NULL;
        }
        HeapFree(GetProcessHeap(), 0, AsyncInet);
    }
}

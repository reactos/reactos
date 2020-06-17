/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Async Internet operation using WinINet
 * COPYRIGHT:   Copyright 2020 He Yang            (1160386205@qq.com)
 */

#include "rapps.h"
#include <windows.h>
#include <wininet.h>
#include <atlbase.h>
#include "asyncinet.h"

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

    AsyncInet->bCleanUp = FALSE;
    AsyncInet->bCancelled = FALSE;

    AsyncInet->hInternet = NULL;
    AsyncInet->hInetFile = NULL;

    AsyncInet->Callback = Callback;
    AsyncInet->Extension = Extension;

    AsyncInet->hEventHandleCreated = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection(&(AsyncInet->CriticalSection));
    AsyncInet->PendingIOCnt = 0;
    AsyncInet->hEventNoPending = CreateEvent(NULL, TRUE, TRUE, NULL);
    AsyncInet->hEventHandleClose = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (AsyncInet->hEventHandleCreated && AsyncInet->hEventNoPending && AsyncInet->hEventHandleClose)
    {
        AsyncInet->hInternet = InternetOpenW(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, INTERNET_FLAG_ASYNC);

        if (AsyncInet->hInternet)
        {
            OldCallbackFunc = InternetSetStatusCallbackW(AsyncInet->hInternet, AsyncInetStatusCallback);
            if (OldCallbackFunc != INTERNET_INVALID_STATUS_CALLBACK)
            {
                InetOpenUrlFlag |= INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PASSIVE | INTERNET_FLAG_RESYNCHRONIZE;
                if (!bAllowCache)
                {
                    InetOpenUrlFlag |= INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD;
                }

                if (AsyncInetAcquire(AsyncInet))
                {
                    AsyncInet->hInetFile = InternetOpenUrlW(AsyncInet->hInternet, lpszUrl, 0, 0, InetOpenUrlFlag, (DWORD_PTR)AsyncInet);

                    if (AsyncInet->hInetFile)
                    {
                        // operate complete synchronously
                        bSuccess = TRUE;
                        AsyncInetReadFileLoop(AsyncInet);
                        AsyncInetRelease(AsyncInet);
                    }
                    else
                    {
                        if (GetLastError() == ERROR_IO_PENDING)
                        {
                            // everything fine. waiting for handle created
                            switch (WaitForSingleObject(AsyncInet->hEventHandleCreated, INFINITE))
                            {
                            case WAIT_OBJECT_0:
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

    return AsyncInet;
}

BOOL AsyncInetCancel(pASYNCINET AsyncInet) // mark as cancelled (this will send a cancel notificaion at last) and do graceful cleanup.
{
    if (AsyncInet)
    {
        AsyncInet->bCancelled = TRUE;
        return AsyncInetCleanUp(AsyncInet);
    }
    else
    {
        return FALSE;
    }
}

BOOL AsyncInetAcquire(pASYNCINET AsyncInet) // try to increase refcnt by 1. if returned FALSE, AsyncInet should not be used anymore
{
    BOOL bResult = FALSE;
    if (AsyncInet)
    {
        EnterCriticalSection(&(AsyncInet->CriticalSection));
        if (!(AsyncInet->bCleanUp))
        {
            AsyncInet->PendingIOCnt++;
            ResetEvent(AsyncInet->hEventNoPending); // no longer zero
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
    if (AsyncInet)
    {
        EnterCriticalSection(&(AsyncInet->CriticalSection));
        if (AsyncInet->PendingIOCnt)
        {
            AsyncInet->PendingIOCnt--;
            if (AsyncInet->PendingIOCnt == 0)
            {
                SetEvent(AsyncInet->hEventNoPending);
            }
        }
        else
        {
            ATLASSERT(FALSE); // should be always non-negative, can not decrease anymore.
        }
        LeaveCriticalSection(&(AsyncInet->CriticalSection));
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
        SetEvent(AsyncInet->hEventHandleClose);
        break;
    }
    case INTERNET_STATUS_REQUEST_COMPLETE:
    {
        INTERNET_ASYNC_RESULT* AsyncResult = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;

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
                AsyncInetRelease(AsyncInet);
            }
            else // asynchronous InternetReadFile complete
            {
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_DATA, (WPARAM)(AsyncInet->ReadBuffer), (LPARAM)(AsyncInet->BytesRead));
                AsyncInetRelease(AsyncInet);
            }

            AsyncInetReadFileLoop(AsyncInet);
        }
        break;
        case ERROR_INVALID_HANDLE:
        case ERROR_INTERNET_OPERATION_CANCELLED:
            if (AsyncInet->bCleanUp)
            {
                AsyncInetRelease(AsyncInet);
                break;
            }

            // fall down
        default:
            // something went wrong
            AsyncInetPerformCallback(AsyncInet, ASYNCINET_ERROR, 0, (LPARAM)(AsyncResult->dwError));
            AsyncInetRelease(AsyncInet);
            AsyncInetCleanUp(AsyncInet);
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
        if (!AsyncInetAcquire(AsyncInet))
        {
            // abort now.
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
                // all read.
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_COMPLETE, 0, 0);
                AsyncInetRelease(AsyncInet);
                AsyncInetCleanUp(AsyncInet);
                break;
            }
            else
            {
                // read completed immediately.
                AsyncInetRelease(AsyncInet);
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
                if ((dwError == ERROR_INVALID_HANDLE || dwError == ERROR_INTERNET_OPERATION_CANCELLED) && AsyncInet->bCleanUp)
                {
                    // not an error. just normally cancelling
                }
                else
                {
                    AsyncInetPerformCallback(AsyncInet, ASYNCINET_ERROR, 0, dwError);
                }

                AsyncInetRelease(AsyncInet);
                AsyncInetCleanUp(AsyncInet);
            }
        }
    }
    return;
}

BOOL AsyncInetCleanUp(pASYNCINET AsyncInet) // gracefully cancel operation and clean up
{
    if (AsyncInet)
    {
        // do not allow using AsyncInet anymore. 
        // this will make all subsequence AsyncInetAcquire return FALSE.
        EnterCriticalSection(&(AsyncInet->CriticalSection));
        if (AsyncInet->bCleanUp)
        {
            // already in progress of cleanup
            LeaveCriticalSection(&(AsyncInet->CriticalSection));
            return FALSE;
        }
        AsyncInet->bCleanUp = TRUE;
        LeaveCriticalSection(&(AsyncInet->CriticalSection));

        // close the handle, waiting for all pending request cancelled.
        InternetCloseHandle(AsyncInet->hInetFile);
        AsyncInet->hInetFile = NULL;

        HANDLE WaitHandleList[] = { AsyncInet->hEventNoPending , AsyncInet->hEventHandleClose };
        // only cleanup when handle closed and refcnt == 0
        switch (WaitForMultipleObjects(_countof(WaitHandleList), WaitHandleList, TRUE, INFINITE))
        {
        case WAIT_OBJECT_0:
        case WAIT_OBJECT_0 + 1:
        {
            if (AsyncInet->bCancelled)
            {
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_CANCELLED, 0, 0);
            }
            AsyncInetFree(AsyncInet);
            return TRUE;
        }
        default:
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
        if (AsyncInet->hEventNoPending)
        {
            CloseHandle(AsyncInet->hEventNoPending);
            AsyncInet->hEventNoPending = NULL;
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


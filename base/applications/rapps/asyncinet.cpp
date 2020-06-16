/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Async Internet operation using WinINet
 * COPYRIGHT:   Copyright 2020 He Yang            (1160386205@qq.com)
 */


#include "rapps.h"
#include <windows.h>
#include <wininet.h>
#include <strsafe.h>
#include "asyncinet.h"


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

VOID AsyncInetReadFileLoop(pASYNCINET AsyncInet)
{
    if ((!AsyncInet) || (!AsyncInet->hInetFile))
    {
        return;
    }

    while (1)
    {
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

                // clean up, close handles.

                // AsyncInet may got freed while handling INTERNET_STATUS_HANDLE_CLOSING.
                // so store the handle first, then close them
                HINTERNET hInetFile = AsyncInet->hInetFile;
                HINTERNET hInternet = AsyncInet->hInternet;
                if (hInetFile)
                {
                    InternetCloseHandle(hInetFile);
                }
                if (hInternet)
                {
                    InternetCloseHandle(hInternet);
                }
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
                //something went wrong
                if (dwError == ERROR_INVALID_HANDLE || dwError == ERROR_INTERNET_OPERATION_CANCELLED)
                {
                    if (AsyncInet->bIsCancelled)
                    {
                        // That's most likely not an error. handle got closed when canceling may lead to this
                        break;
                    }
                }
                AsyncInetPerformCallback(AsyncInet, ASYNCINET_ERROR, 0, (LPARAM)dwError);
                break;
            }
        }
    }
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
        break;
    }
    case INTERNET_STATUS_HANDLE_CLOSING:
    {
        if (AsyncInet->bIsCancelled)
        {
            AsyncInetPerformCallback(AsyncInet, ASYNCINET_CANCELLED, 0, 0);
        }
        // handle now closed. free the memory.
        HeapFree(GetProcessHeap(), 0, AsyncInet);
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
            if (AsyncInet->bIsCancelled)
            {
                // this is most likely not an error. when canceling, using closed/invalid handle may lead to this
                break;
            }
            // fall-down to handle error
        default:
            // something went wrong
            AsyncInetPerformCallback(AsyncInet, ASYNCINET_ERROR, 0, (LPARAM)(AsyncResult->dwError));
            break;
        }
        break;
    }

    }
    return;
}


pASYNCINET AsyncInetDownload(LPCWSTR lpszAgent,
    DWORD   dwAccessType,
    LPCWSTR lpszProxy,
    LPCWSTR lpszProxyBypass,
    LPCWSTR lpszUrl,
    BOOL bAllowCache,
    ASYNCINET_CALLBACK Callback,
    VOID* Extension
    )
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

    AsyncInet->Callback = Callback;
    AsyncInet->Extension = Extension;

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
                    // everything fine
                    bSuccess = TRUE;
                }
            }
        }
    }

    if (!bSuccess)
    {
        if (AsyncInet)
        {
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
            AsyncInet = NULL;
        }
    }

    return AsyncInet;
}


BOOL AsyncInetCancel(pASYNCINET AsyncInet)
{
    if (!AsyncInet)
    {
        return FALSE;
    }

    AsyncInet->bIsCancelled = TRUE;

    // AsyncInet may got freed while handling INTERNET_STATUS_HANDLE_CLOSING.
    // so store the handle first, then close them

    HINTERNET hInetFile = AsyncInet->hInetFile;
    HINTERNET hInternet = AsyncInet->hInternet;
    if (hInetFile)
    {
        InternetCloseHandle(hInetFile);
    }
    if (hInternet)
    {
        InternetCloseHandle(hInternet);
    }

    return TRUE;
}

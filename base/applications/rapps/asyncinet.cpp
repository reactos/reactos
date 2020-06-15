/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/asyncinet.cpp
 * PURPOSE:     Async Internet operation using WinINet
 * COPYRIGHT:   Copyright 2020 He Yang            (1160386205@qq.com)
 */


#include <windows.h>
#include <wininet.h>
#include <strsafe.h>
#include "asyncinet.h"


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
            if (AsyncInet->Callback)
            {
                AsyncInet->Callback(AsyncInet, ASYNCINET_CANCELLED, 0, 0, AsyncInet->Extension);
            }
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
                if (AsyncInet->Callback)
                {
                    AsyncInet->Callback(AsyncInet, ASYNCINET_DATA, (WPARAM)(AsyncInet->ReadBuffer), (LPARAM)(AsyncInet->BytesRead), AsyncInet->Extension);
                }
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
                        if (AsyncInet->Callback)
                        {
                            AsyncInet->Callback(AsyncInet, ASYNCINET_COMPLETE, 0, 0, AsyncInet->Extension);

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
                    }
                    else
                    {
                        // read completed immediately.
                        if (AsyncInet->Callback)
                        {
                            AsyncInet->Callback(AsyncInet, ASYNCINET_DATA, (WPARAM)(AsyncInet->ReadBuffer), (LPARAM)(AsyncInet->BytesRead), AsyncInet->Extension);
                        }
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
                        if (dwError == ERROR_INVALID_HANDLE)
                        {
                            // That's most likely not an error. handle got closed when canceling may lead to this
                            break;
                        }
                        if (AsyncInet->Callback)
                        {
                            AsyncInet->Callback(AsyncInet, ASYNCINET_ERROR, 0, (LPARAM)dwError, AsyncInet->Extension);
                        }
                        break;
                    }
                }
            }
        }
        break;
        case ERROR_INVALID_HANDLE:
            // this is most likely not an error. when canceling, using closed/invalid handle may lead to this
            break;
        default:
            // something went wrong
            if (AsyncInet->Callback)
            {
                AsyncInet->Callback(AsyncInet, ASYNCINET_ERROR, 0, (LPARAM)(AsyncResult->dwError), AsyncInet->Extension);
            }
            break;
        }
        break;
    }

    }
    return;
}


pASYNCINET AsyncInetDownloadW(LPCWSTR lpszAgent,
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
    if (!AsyncInet)goto cleanup;

    AsyncInet->Callback = Callback;
    AsyncInet->Extension = Extension;

    AsyncInet->hInternet = InternetOpenW(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, INTERNET_FLAG_ASYNC);
    if (!AsyncInet->hInternet)goto cleanup;

    OldCallbackFunc = InternetSetStatusCallbackW(AsyncInet->hInternet, AsyncInetStatusCallback);
    if (OldCallbackFunc == INTERNET_INVALID_STATUS_CALLBACK)goto cleanup;

    InetOpenUrlFlag |= INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PASSIVE;
    if (!bAllowCache)
    {
        InetOpenUrlFlag |= INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE;
    }
    AsyncInet->hInetFile = InternetOpenUrlW(AsyncInet->hInternet, lpszUrl, 0, 0, InetOpenUrlFlag, (DWORD_PTR)AsyncInet);
    if (!AsyncInet->hInetFile)
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            goto cleanup;
        }
    }
    else
    {
        // TODO: If I remember it correctly, sometimes the file is cache before
        // thus may lead to InternetOpenUrlW return immediately with the handle.
        // more investigation required. And if it's true, it should be handled correctly.
    }

    bSuccess = TRUE;


cleanup:

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

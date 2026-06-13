/*
 * Internet Messaging Transport Base Class
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include "ws2tcpip.h"
#include "windef.h"
#include "winnt.h"
#include "objbase.h"
#include "ole2.h"
#include "mimeole.h"

#include <stdio.h>

#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

static const WCHAR wszClassName[] = L"ThorConnWndClass";

#define IX_READ     (WM_USER + 0)
#define IX_READLINE (WM_USER + 1)
#define IX_WRITE    (WM_USER + 2)

HRESULT InternetTransport_Init(InternetTransport *This)
{
    This->pCallback = NULL;
    This->Status = IXP_DISCONNECTED;
    This->Socket = -1;
    This->fCommandLogging = FALSE;
    This->fnCompletion = NULL;

    return S_OK;
}

HRESULT InternetTransport_GetServerInfo(InternetTransport *This, LPINETSERVER pInetServer)
{
    if (This->Status == IXP_DISCONNECTED)
        return IXP_E_NOT_CONNECTED;

    *pInetServer = This->ServerInfo;
    return S_OK;
}

HRESULT InternetTransport_InetServerFromAccount(InternetTransport *This,
    IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    FIXME("(%p, %p): stub\n", pAccount, pInetServer);
    return E_NOTIMPL;
}

HRESULT InternetTransport_Connect(InternetTransport *This,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    struct addrinfo *ai;
    struct addrinfo *ai_cur;
    struct addrinfo hints;
    int ret;
    char szPort[10];

    if (This->Status != IXP_DISCONNECTED)
        return IXP_E_ALREADY_CONNECTED;

    This->ServerInfo = *pInetServer;
    This->fCommandLogging = fCommandLogging;

    This->hwnd = CreateWindowW(wszClassName, wszClassName, 0, 0, 0, 0, 0, NULL, NULL, NULL, 0);
    if (!This->hwnd)
        return HRESULT_FROM_WIN32(GetLastError());
    SetWindowLongPtrW(This->hwnd, GWLP_USERDATA, (LONG_PTR)This);

    hints.ai_flags          = 0;
    hints.ai_family         = PF_UNSPEC;
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_protocol       = IPPROTO_TCP;
    hints.ai_addrlen        = 0;
    hints.ai_addr           = NULL;
    hints.ai_canonname      = NULL;
    hints.ai_next           = NULL;

    snprintf(szPort, sizeof(szPort), "%d", (unsigned short)pInetServer->dwPort);

    InternetTransport_ChangeStatus(This, IXP_FINDINGHOST);

    ret = getaddrinfo(pInetServer->szServerName, szPort, &hints, &ai);
    if (ret)
    {
        ERR("getaddrinfo failed: %d\n", ret);
        return IXP_E_CANT_FIND_HOST;
    }

    for (ai_cur = ai; ai_cur; ai_cur = ai->ai_next)
    {
        int so;

        if (TRACE_ON(inetcomm))
        {
            char host[256];
            char service[256];
            getnameinfo(ai_cur->ai_addr, ai_cur->ai_addrlen,
                host, sizeof(host), service, sizeof(service),
                NI_NUMERICHOST | NI_NUMERICSERV);
            TRACE("trying %s:%s\n", host, service);
        }

        InternetTransport_ChangeStatus(This, IXP_CONNECTING);

        so = socket(ai_cur->ai_family, ai_cur->ai_socktype, ai_cur->ai_protocol);
        if (so == -1)
        {
            WARN("socket() failed\n");
            continue;
        }
        This->Socket = so;

        /* FIXME: set to async */

        if (0 > connect(This->Socket, ai_cur->ai_addr, ai_cur->ai_addrlen))
        {
            WARN("connect() failed\n");
            closesocket(This->Socket);
            continue;
        }
        InternetTransport_ChangeStatus(This, IXP_CONNECTED);

        /* FIXME: call WSAAsyncSelect */

        freeaddrinfo(ai);
        TRACE("connected\n");
        return S_OK;
    }

    freeaddrinfo(ai);

    return IXP_E_CANT_FIND_HOST;
}

HRESULT InternetTransport_HandsOffCallback(InternetTransport *This)
{
    if (!This->pCallback)
        return S_FALSE;

    ITransportCallback_Release(This->pCallback);
    This->pCallback = NULL;

    return S_OK;
}

HRESULT InternetTransport_DropConnection(InternetTransport *This)
{
    if (This->Status == IXP_DISCONNECTED)
        return IXP_E_NOT_CONNECTED;

    shutdown(This->Socket, SD_BOTH);

    closesocket(This->Socket);

    DestroyWindow(This->hwnd);
    This->hwnd = NULL;

    InternetTransport_ChangeStatus(This, IXP_DISCONNECTED);

    return S_OK;
}

HRESULT InternetTransport_GetStatus(InternetTransport *This,
    IXPSTATUS *pCurrentStatus)
{
    *pCurrentStatus = This->Status;
    return S_OK;
}

HRESULT InternetTransport_ChangeStatus(InternetTransport *This, IXPSTATUS Status)
{
    This->Status = Status;
    if (This->pCallback)
        ITransportCallback_OnStatus(This->pCallback, Status,
            (IInternetTransport *)&This->u.vtbl);
    return S_OK;
}

HRESULT InternetTransport_ReadLine(InternetTransport *This,
    INETXPORT_COMPLETION_FUNCTION fnCompletion)
{
    if (This->Status == IXP_DISCONNECTED)
        return IXP_E_NOT_CONNECTED;

    if (This->fnCompletion)
        return IXP_E_BUSY;

    This->fnCompletion = fnCompletion;

    This->cbBuffer = 1024;
    This->pBuffer = HeapAlloc(GetProcessHeap(), 0, This->cbBuffer);
    This->iCurrentBufferOffset = 0;

    if (WSAAsyncSelect(This->Socket, This->hwnd, IX_READLINE, FD_READ) == SOCKET_ERROR)
    {
        ERR("WSAAsyncSelect failed with error %d\n", WSAGetLastError());
        /* FIXME: handle error */
    }
    return S_OK;
}

HRESULT InternetTransport_Write(InternetTransport *This, const char *pvData,
    int cbSize, INETXPORT_COMPLETION_FUNCTION fnCompletion)
{
    int ret;

    if (This->Status == IXP_DISCONNECTED)
        return IXP_E_NOT_CONNECTED;

    if (This->fnCompletion)
        return IXP_E_BUSY;

    /* FIXME: do this asynchronously */
    ret = send(This->Socket, pvData, cbSize, 0);
    if (ret == SOCKET_ERROR)
    {
        ERR("send failed with error %d\n", WSAGetLastError());
        /* FIXME: handle error */
    }

    fnCompletion((IInternetTransport *)&This->u.vtbl, NULL, 0);

    return S_OK;
}

HRESULT InternetTransport_DoCommand(InternetTransport *This,
    LPCSTR pszCommand, INETXPORT_COMPLETION_FUNCTION fnCompletion)
{
    if (This->Status == IXP_DISCONNECTED)
        return IXP_E_NOT_CONNECTED;

    if (This->fnCompletion)
        return IXP_E_BUSY;

    if (This->pCallback && This->fCommandLogging)
    {
        ITransportCallback_OnCommand(This->pCallback, CMD_SEND, (LPSTR)pszCommand, 0,
            (IInternetTransport *)&This->u.vtbl);
    }
    return InternetTransport_Write(This, pszCommand, strlen(pszCommand), fnCompletion);
}

static LRESULT CALLBACK InternetTransport_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == IX_READ)
    {
        InternetTransport *This = (InternetTransport *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

        /* no work to do */
        if (!This->fnCompletion)
            return 0;

        while (This->iCurrentBufferOffset < This->cbBuffer)
        {
            if (recv(This->Socket, &This->pBuffer[This->iCurrentBufferOffset], 1, 0) <= 0)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    break;

                ERR("recv failed with error %d\n", WSAGetLastError());
                /* FIXME: handle error */
            }

            This->iCurrentBufferOffset++;
        }
        if (This->iCurrentBufferOffset == This->cbBuffer)
        {
            INETXPORT_COMPLETION_FUNCTION fnCompletion = This->fnCompletion;
            char *pBuffer;

            This->fnCompletion = NULL;
            pBuffer = This->pBuffer;
            This->pBuffer = NULL;
            fnCompletion((IInternetTransport *)&This->u.vtbl, pBuffer,
                This->iCurrentBufferOffset);
            HeapFree(GetProcessHeap(), 0, pBuffer);
            return 0;
        }

        if (WSAAsyncSelect(This->Socket, hwnd, uMsg, FD_READ) == SOCKET_ERROR)
        {
            ERR("WSAAsyncSelect failed with error %d\n", WSAGetLastError());
            /* FIXME: handle error */
        }
        return 0;
    }
    else if (uMsg == IX_READLINE)
    {
        InternetTransport *This = (InternetTransport *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

        /* no work to do */
        if (!This->fnCompletion)
            return 0;

        while (This->iCurrentBufferOffset < This->cbBuffer - 1)
        {
            fd_set infd;

            if (recv(This->Socket, &This->pBuffer[This->iCurrentBufferOffset], 1, 0) <= 0)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    break;

                ERR("recv failed with error %d\n", WSAGetLastError());
                /* FIXME: handle error */
                return 0;
            }

            if (This->pBuffer[This->iCurrentBufferOffset] == '\n')
            {
                INETXPORT_COMPLETION_FUNCTION fnCompletion = This->fnCompletion;
                char *pBuffer;

                This->fnCompletion = NULL;
                This->pBuffer[This->iCurrentBufferOffset++] = '\0';
                pBuffer = This->pBuffer;
                This->pBuffer = NULL;

                fnCompletion((IInternetTransport *)&This->u.vtbl, pBuffer,
                    This->iCurrentBufferOffset);

                HeapFree(GetProcessHeap(), 0, pBuffer);
                return 0;
            }
            if (This->pBuffer[This->iCurrentBufferOffset] != '\r')
                This->iCurrentBufferOffset++;

            FD_ZERO(&infd);
            FD_SET(This->Socket, &infd);
        }
        if (This->iCurrentBufferOffset == This->cbBuffer - 1)
            return 0;

        if (WSAAsyncSelect(This->Socket, hwnd, uMsg, FD_READ) == SOCKET_ERROR)
        {
            ERR("WSAAsyncSelect failed with error %d\n", WSAGetLastError());
            /* FIXME: handle error */
        }
        return 0;
    }
    else
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL InternetTransport_RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSW cls;
    WSADATA wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata))
        return FALSE;

    memset(&cls, 0, sizeof(cls));
    cls.hInstance     = hInstance;
    cls.lpfnWndProc   = InternetTransport_WndProc;
    cls.lpszClassName = wszClassName;

    return RegisterClassW(&cls);
}

void InternetTransport_UnregisterClass(HINSTANCE hInstance)
{
    UnregisterClassW(wszClassName, hInstance);
    WSACleanup();
}

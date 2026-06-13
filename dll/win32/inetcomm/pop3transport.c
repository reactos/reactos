/*
 * POP3 Transport
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winuser.h"
#include "objbase.h"
#include "mimeole.h"
#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

enum parse_state
{
    STATE_NONE,
    STATE_OK,
    STATE_MULTILINE,
    STATE_DONE
};

typedef struct
{
    InternetTransport InetTransport;
    ULONG refs;
    POP3COMMAND command;
    POP3CMDTYPE type;
    char *response;
    char *ptr;
    enum parse_state state;
    BOOL valid_info;
    DWORD msgid;
    DWORD preview_lines;
} POP3Transport;

static HRESULT parse_response(POP3Transport *This)
{
    switch (This->state)
    {
    case STATE_NONE:
        if (strlen(This->response) < 3)
        {
            WARN("parse error\n");
            This->state = STATE_DONE;
            return S_FALSE;
        }
        if (!memcmp(This->response, "+OK", 3))
        {
            This->ptr = This->response + 3;
            This->state = STATE_OK;
            return S_OK;
        }
        This->state = STATE_DONE;
        return S_FALSE;

    default: return S_OK;
    }
}

static HRESULT parse_uidl_response(POP3Transport *This, POP3UIDL *uidl)
{
    char *p;

    uidl->dwPopId = 0;
    uidl->pszUidl = NULL;
    switch (This->state)
    {
    case STATE_OK:
        if (This->type == POP3CMD_GET_POPID)
        {
            if ((p = strchr(This->ptr, ' ')))
            {
                while (*p == ' ') p++;
                sscanf(p, "%lu", &uidl->dwPopId);
                if ((p = strchr(p, ' ')))
                {
                    while (*p == ' ') p++;
                    uidl->pszUidl = p;
                    This->valid_info = TRUE;
                }
             }
             This->state = STATE_DONE;
             return S_OK;
        }
        This->state = STATE_MULTILINE;
        return S_OK;

    case STATE_MULTILINE:
        if (This->response[0] == '.' && !This->response[1])
        {
            This->valid_info = FALSE;
            This->state = STATE_DONE;
            return S_OK;
        }
        sscanf(This->response, "%lu", &uidl->dwPopId);
        if ((p = strchr(This->response, ' ')))
        {
            while (*p == ' ') p++;
            uidl->pszUidl = p;
            This->valid_info = TRUE;
            return S_OK;
        }

    default:
        WARN("parse error\n");
        This->state = STATE_DONE;
        return S_FALSE;
    }
}

static HRESULT parse_stat_response(POP3Transport *This, POP3STAT *stat)
{
    char *p;

    stat->cMessages = 0;
    stat->cbMessages = 0;
    switch (This->state)
    {
    case STATE_OK:
        if ((p = strchr(This->ptr, ' ')))
        {
            while (*p == ' ') p++;
            sscanf(p, "%lu %lu", &stat->cMessages, &stat->cbMessages);
            This->valid_info = TRUE;
            This->state = STATE_DONE;
            return S_OK;
        }

    default:
        WARN("parse error\n");
        This->state = STATE_DONE;
        return S_FALSE;
    }
}

static HRESULT parse_list_response(POP3Transport *This, POP3LIST *list)
{
    char *p;

    list->dwPopId = 0;
    list->cbSize = 0;
    switch (This->state)
    {
    case STATE_OK:
        if (This->type == POP3CMD_GET_POPID)
        {
            if ((p = strchr(This->ptr, ' ')))
            {
                while (*p == ' ') p++;
                sscanf(p, "%lu %lu", &list->dwPopId, &list->cbSize);
                This->valid_info = TRUE;
            }
            This->state = STATE_DONE;
            return S_OK;
        }
        This->state = STATE_MULTILINE;
        return S_OK;

    case STATE_MULTILINE:
        if (This->response[0] == '.' && !This->response[1])
        {
            This->valid_info = FALSE;
            This->state = STATE_DONE;
            return S_OK;
        }
        sscanf(This->response, "%lu", &list->dwPopId);
        if ((p = strchr(This->response, ' ')))
        {
            while (*p == ' ') p++;
            sscanf(p, "%lu", &list->cbSize);
            This->valid_info = TRUE;
            return S_OK;
        }

    default:
        WARN("parse error\n");
        This->state = STATE_DONE;
        return S_FALSE;
    }
}

static HRESULT parse_dele_response(POP3Transport *This, DWORD *dwPopId)
{
    switch (This->state)
    {
    case STATE_OK:
        *dwPopId = 0; /* FIXME */
        This->state = STATE_DONE;
        return S_OK;

    default:
        WARN("parse error\n");
        This->state = STATE_DONE;
        return S_FALSE;
    }
}

static HRESULT parse_retr_response(POP3Transport *This, POP3RETR *retr)
{
    switch (This->state)
    {
    case STATE_OK:
        retr->fHeader = FALSE;
        retr->fBody = FALSE;
        retr->dwPopId = This->msgid;
        retr->cbSoFar = 0;
        retr->pszLines = This->response;
        retr->cbLines = 0;

        This->state = STATE_MULTILINE;
        This->valid_info = FALSE;
        return S_OK;

    case STATE_MULTILINE:
    {
        int len;

        if (This->response[0] == '.' && !This->response[1])
        {
            retr->cbLines = retr->cbSoFar;
            This->state = STATE_DONE;
            return S_OK;
        }
        retr->fHeader = TRUE;
        if (!This->response[0]) retr->fBody = TRUE;

        len = strlen(This->response);
        retr->cbSoFar += len;
        retr->pszLines = This->response;
        retr->cbLines = len;

        This->valid_info = TRUE;
        return S_OK;
    }

    default:
        WARN("parse error\n");
        This->state = STATE_DONE;
        return S_FALSE;
    }
}

static HRESULT parse_top_response(POP3Transport *This, POP3TOP *top)
{
    switch (This->state)
    {
    case STATE_OK:
        top->fHeader = FALSE;
        top->fBody = FALSE;
        top->dwPopId = This->msgid;
        top->cPreviewLines = This->preview_lines;
        top->cbSoFar = 0;
        top->pszLines = This->response;
        top->cbLines = 0;

        This->state = STATE_MULTILINE;
        This->valid_info = FALSE;
        return S_OK;

    case STATE_MULTILINE:
    {
        int len;

        if (This->response[0] == '.' && !This->response[1])
        {
            top->cbLines = top->cbSoFar;
            This->state = STATE_DONE;
            return S_OK;
        }
        top->fHeader = TRUE;
        if (!This->response[0]) top->fBody = TRUE;

        len = strlen(This->response);
        top->cbSoFar += len;
        top->pszLines = This->response;
        top->cbLines = len;

        This->valid_info = TRUE;
        return S_OK;
    }

    default:
        WARN("parse error\n");
        This->state = STATE_DONE;
        return S_FALSE;
    }
}

static void init_parser(POP3Transport *This, POP3COMMAND command)
{
    This->state = STATE_NONE;
    This->command = command;
}

static HRESULT POP3Transport_ParseResponse(POP3Transport *This, char *pszResponse, POP3RESPONSE *pResponse)
{
    HRESULT hr;

    TRACE("response: %s\n", debugstr_a(pszResponse));

    This->response = pszResponse;
    This->valid_info = FALSE;
    TRACE("state %u\n", This->state);

    if (SUCCEEDED((hr = parse_response(This))))
    {
        switch (This->command)
        {
        case POP3_UIDL: hr = parse_uidl_response(This, &pResponse->rUidlInfo); break;
        case POP3_STAT: hr = parse_stat_response(This, &pResponse->rStatInfo); break;
        case POP3_LIST: hr = parse_list_response(This, &pResponse->rListInfo); break;
        case POP3_DELE: hr = parse_dele_response(This, &pResponse->dwPopId); break;
        case POP3_RETR: hr = parse_retr_response(This, &pResponse->rRetrInfo); break;
        case POP3_TOP: hr = parse_top_response(This, &pResponse->rTopInfo); break;
        default:
            This->state = STATE_DONE;
            break;
        }
    }
    pResponse->command = This->command;
    pResponse->fDone = (This->state == STATE_DONE);
    pResponse->fValidInfo = This->valid_info;
    pResponse->rIxpResult.hrResult = hr;
    pResponse->rIxpResult.pszResponse = pszResponse;
    pResponse->rIxpResult.uiServerError = 0;
    pResponse->rIxpResult.hrServerError = pResponse->rIxpResult.hrResult;
    pResponse->rIxpResult.dwSocketError = WSAGetLastError();
    pResponse->rIxpResult.pszProblem = NULL;
    pResponse->pTransport = (IPOP3Transport *)&This->InetTransport.u.vtblPOP3;

    if (This->InetTransport.pCallback && This->InetTransport.fCommandLogging)
    {
        ITransportCallback_OnCommand(This->InetTransport.pCallback, CMD_RESP,
            pResponse->rIxpResult.pszResponse, pResponse->rIxpResult.hrServerError,
            (IInternetTransport *)&This->InetTransport.u.vtbl);
    }
    return S_OK;
}

static void POP3Transport_CallbackProcessDELEResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvDELEResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessDELEResp);
}

static void POP3Transport_CallbackProcessNOOPResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvNOOPResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessNOOPResp);
}

static void POP3Transport_CallbackProcessRSETResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvRSETResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessRSETResp);
}

static void POP3Transport_CallbackProcessRETRResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);

    if (!response.fDone)
    {
        InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessRETRResp);
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvRETRResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessRETRResp);
}

static void POP3Transport_CallbackProcessTOPResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);

    if (!response.fDone)
    {
        InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessTOPResp);
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvTOPResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessTOPResp);
}

static void POP3Transport_CallbackProcessLISTResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);

    if (!response.fDone)
    {
        InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessLISTResp);
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvLISTResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessLISTResp);
}

static void POP3Transport_CallbackProcessUIDLResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);

    if (!response.fDone)
    {
        InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessUIDLResp);
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvUIDLResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessUIDLResp);
}

static void POP3Transport_CallbackProcessSTATResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvSTATResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessSTATResp);
}

static void POP3Transport_CallbackProcessPASSResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    InternetTransport_ChangeStatus(&This->InetTransport, IXP_AUTHORIZED);
    InternetTransport_ChangeStatus(&This->InetTransport, IXP_CONNECTED);

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
}

static void POP3Transport_CallbackRecvPASSResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessPASSResp);
}

static void POP3Transport_CallbackProcessUSERResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    static const char pass[] = "PASS ";
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    char *command;
    int len;
    HRESULT hr;

    TRACE("\n");

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);

    len = sizeof(pass) + strlen(This->InetTransport.ServerInfo.szPassword) + 2; /* "\r\n" */
    command = HeapAlloc(GetProcessHeap(), 0, len);

    strcpy(command, pass);
    strcat(command, This->InetTransport.ServerInfo.szPassword);
    strcat(command, "\r\n");

    init_parser(This, POP3_PASS);

    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvPASSResp);
    HeapFree(GetProcessHeap(), 0, command);
}

static void POP3Transport_CallbackRecvUSERResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessUSERResp);
}

static void POP3Transport_CallbackSendUSERCmd(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    static const char user[] = "USER ";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("\n");

    len = sizeof(user) + strlen(This->InetTransport.ServerInfo.szUserName) + 2; /* "\r\n" */
    command = HeapAlloc(GetProcessHeap(), 0, len);

    strcpy(command, user);
    strcat(command, This->InetTransport.ServerInfo.szUserName);
    strcat(command, "\r\n");
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvUSERResp);

    HeapFree(GetProcessHeap(), 0, command);
}

static void POP3Transport_CallbackProcessQUITResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;
    POP3RESPONSE response;
    HRESULT hr;

    TRACE("%s\n", debugstr_an(pBuffer, cbBuffer));

    hr = POP3Transport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    IPOP3Callback_OnResponse((IPOP3Callback *)This->InetTransport.pCallback, &response);
    InternetTransport_DropConnection(&This->InetTransport);
}

static void POP3Transport_CallbackRecvQUITResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackProcessQUITResponse);
}

static HRESULT WINAPI POP3Transport_QueryInterface(IPOP3Transport *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IInternetTransport) ||
        IsEqualIID(riid, &IID_IPOP3Transport))
    {
        *ppv = iface;
        IPOP3Transport_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    FIXME("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI POP3Transport_AddRef(IPOP3Transport *iface)
{
    POP3Transport *This = (POP3Transport *)iface;
    return InterlockedIncrement((LONG *)&This->refs);
}

static ULONG WINAPI POP3Transport_Release(IPOP3Transport *iface)
{
    POP3Transport *This = (POP3Transport *)iface;
    ULONG refs = InterlockedDecrement((LONG *)&This->refs);
    if (!refs)
    {
        TRACE("destroying %p\n", This);
        if (This->InetTransport.Status != IXP_DISCONNECTED)
            InternetTransport_DropConnection(&This->InetTransport);
        if (This->InetTransport.pCallback) ITransportCallback_Release(This->InetTransport.pCallback);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI POP3Transport_GetServerInfo(IPOP3Transport *iface,
    LPINETSERVER pInetServer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("(%p)\n", pInetServer);
    return InternetTransport_GetServerInfo(&This->InetTransport, pInetServer);
}

static IXPTYPE WINAPI POP3Transport_GetIXPType(IPOP3Transport *iface)
{
    TRACE("()\n");
    return IXP_POP3;
}

static HRESULT WINAPI POP3Transport_IsState(IPOP3Transport *iface, IXPISSTATE isstate)
{
    FIXME("(%u)\n", isstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI POP3Transport_InetServerFromAccount(
    IPOP3Transport *iface, IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("(%p, %p)\n", pAccount, pInetServer);
    return InternetTransport_InetServerFromAccount(&This->InetTransport, pAccount, pInetServer);
}

static HRESULT WINAPI POP3Transport_Connect(IPOP3Transport *iface,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    POP3Transport *This = (POP3Transport *)iface;
    HRESULT hr;

    TRACE("(%p, %s, %s)\n", pInetServer, fAuthenticate ? "TRUE" : "FALSE", fCommandLogging ? "TRUE" : "FALSE");

    hr = InternetTransport_Connect(&This->InetTransport, pInetServer, fAuthenticate, fCommandLogging);
    if (FAILED(hr))
        return hr;

    init_parser(This, POP3_USER);
    return InternetTransport_ReadLine(&This->InetTransport, POP3Transport_CallbackSendUSERCmd);
}

static HRESULT WINAPI POP3Transport_HandsOffCallback(IPOP3Transport *iface)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("()\n");
    return InternetTransport_HandsOffCallback(&This->InetTransport);
}

static HRESULT WINAPI POP3Transport_Disconnect(IPOP3Transport *iface)
{
    TRACE("()\n");
    return IPOP3Transport_CommandQUIT(iface);
}

static HRESULT WINAPI POP3Transport_DropConnection(IPOP3Transport *iface)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("()\n");
    return InternetTransport_DropConnection(&This->InetTransport);
}

static HRESULT WINAPI POP3Transport_GetStatus(IPOP3Transport *iface,
    IXPSTATUS *pCurrentStatus)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("()\n");
    return InternetTransport_GetStatus(&This->InetTransport, pCurrentStatus);
}

static HRESULT WINAPI POP3Transport_InitNew(IPOP3Transport *iface,
    LPSTR pszLogFilePath, IPOP3Callback *pCallback)
{
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("(%s, %p)\n", debugstr_a(pszLogFilePath), pCallback);

    if (!pCallback)
        return E_INVALIDARG;

    if (pszLogFilePath)
        FIXME("not using log file of %s, use Wine debug logging instead\n",
            debugstr_a(pszLogFilePath));

    IPOP3Callback_AddRef(pCallback);
    This->InetTransport.pCallback = (ITransportCallback *)pCallback;
    This->InetTransport.fInitialised = TRUE;

    return S_OK;
}

static HRESULT WINAPI POP3Transport_MarkItem(IPOP3Transport *iface, POP3MARKTYPE marktype,
    DWORD dwPopId, boolean fMarked)
{
    FIXME("(%u, %lu, %d)\n", marktype, dwPopId, fMarked);
    return E_NOTIMPL;
}

static HRESULT WINAPI POP3Transport_CommandAUTH(IPOP3Transport *iface, LPSTR pszAuthType)
{
    FIXME("(%s)\n", pszAuthType);
    return E_NOTIMPL;
}

static HRESULT WINAPI POP3Transport_CommandUSER(IPOP3Transport *iface, LPSTR username)
{
    static const char user[] = "USER ";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%s)\n", username);

    len = sizeof(user) + strlen(username) + 2; /* "\r\n" */
    if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;

    strcpy(command, user);
    strcat(command, username);
    strcat(command, "\r\n");

    init_parser(This, POP3_USER);
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvUSERResp);

    HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandPASS(IPOP3Transport *iface, LPSTR password)
{
    static const char pass[] = "PASS ";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%p)\n", password);

    len = sizeof(pass) + strlen(password) + 2; /* "\r\n" */
    if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;

    strcpy(command, pass);
    strcat(command, password);
    strcat(command, "\r\n");

    init_parser(This, POP3_PASS);
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvPASSResp);

    HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandLIST(
    IPOP3Transport *iface, POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    static const char list[] = "LIST %lu\r\n";
    static char list_all[] = "LIST\r\n";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%u, %lu)\n", cmdtype, dwPopId);

    if (dwPopId)
    {
        len = sizeof(list) + 10 + 2; /* "4294967296" + "\r\n" */
        if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;
        sprintf(command, list, dwPopId);
    }
    else command = list_all;

    init_parser(This, POP3_LIST);
    This->type = cmdtype;
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvLISTResp);

    if (dwPopId) HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandTOP(
    IPOP3Transport *iface, POP3CMDTYPE cmdtype, DWORD dwPopId, DWORD cPreviewLines)
{
    static const char top[] = "TOP %lu %lu\r\n";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%u, %lu, %lu)\n", cmdtype, dwPopId, cPreviewLines);

    len = sizeof(top) + 20 + 2; /* 2 * "4294967296" + "\r\n" */
    if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;
    sprintf(command, top, dwPopId, cPreviewLines);

    This->preview_lines = cPreviewLines;
    init_parser(This, POP3_TOP);
    This->type = cmdtype;
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvTOPResp);

    HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandQUIT(IPOP3Transport *iface)
{
    static const char command[] = "QUIT\r\n";
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("()\n");

    InternetTransport_ChangeStatus(&This->InetTransport, IXP_DISCONNECTING);

    init_parser(This, POP3_QUIT);
    return InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvQUITResp);
}

static HRESULT WINAPI POP3Transport_CommandSTAT(IPOP3Transport *iface)
{
    static const char stat[] = "STAT\r\n";
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");

    init_parser(This, POP3_STAT);
    InternetTransport_DoCommand(&This->InetTransport, stat, POP3Transport_CallbackRecvSTATResp);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandNOOP(IPOP3Transport *iface)
{
    static const char noop[] = "NOOP\r\n";
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");

    init_parser(This, POP3_NOOP);
    InternetTransport_DoCommand(&This->InetTransport, noop, POP3Transport_CallbackRecvNOOPResp);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandRSET(IPOP3Transport *iface)
{
    static const char rset[] = "RSET\r\n";
    POP3Transport *This = (POP3Transport *)iface;

    TRACE("\n");

    init_parser(This, POP3_RSET);
    InternetTransport_DoCommand(&This->InetTransport, rset, POP3Transport_CallbackRecvRSETResp);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandUIDL(
    IPOP3Transport *iface, POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    static const char uidl[] = "UIDL %lu\r\n";
    static char uidl_all[] = "UIDL\r\n";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%u, %lu)\n", cmdtype, dwPopId);

    if (dwPopId)
    {
        len = sizeof(uidl) + 10 + 2; /* "4294967296" + "\r\n" */
        if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;
        sprintf(command, uidl, dwPopId);
    }
    else command = uidl_all;

    init_parser(This, POP3_UIDL);
    This->type = cmdtype;
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvUIDLResp);

    if (dwPopId) HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandDELE(
    IPOP3Transport *iface, POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    static const char dele[] = "DELE %lu\r\n";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%u, %lu)\n", cmdtype, dwPopId);

    len = sizeof(dele) + 10 + 2; /* "4294967296" + "\r\n" */
    if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;
    sprintf(command, dele, dwPopId);

    init_parser(This, POP3_DELE);
    This->type = cmdtype;
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvDELEResp);

    HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static HRESULT WINAPI POP3Transport_CommandRETR(
    IPOP3Transport *iface, POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    static const char retr[] = "RETR %lu\r\n";
    POP3Transport *This = (POP3Transport *)iface;
    char *command;
    int len;

    TRACE("(%u, %lu)\n", cmdtype, dwPopId);

    len = sizeof(retr) + 10 + 2; /* "4294967296" + "\r\n" */
    if (!(command = HeapAlloc(GetProcessHeap(), 0, len))) return S_FALSE;
    sprintf(command, retr, dwPopId);

    init_parser(This, POP3_RETR);
    This->type = cmdtype;
    InternetTransport_DoCommand(&This->InetTransport, command, POP3Transport_CallbackRecvRETRResp);

    HeapFree(GetProcessHeap(), 0, command);
    return S_OK;
}

static const IPOP3TransportVtbl POP3TransportVtbl =
{
    POP3Transport_QueryInterface,
    POP3Transport_AddRef,
    POP3Transport_Release,
    POP3Transport_GetServerInfo,
    POP3Transport_GetIXPType,
    POP3Transport_IsState,
    POP3Transport_InetServerFromAccount,
    POP3Transport_Connect,
    POP3Transport_HandsOffCallback,
    POP3Transport_Disconnect,
    POP3Transport_DropConnection,
    POP3Transport_GetStatus,
    POP3Transport_InitNew,
    POP3Transport_MarkItem,
    POP3Transport_CommandAUTH,
    POP3Transport_CommandUSER,
    POP3Transport_CommandPASS,
    POP3Transport_CommandLIST,
    POP3Transport_CommandTOP,
    POP3Transport_CommandQUIT,
    POP3Transport_CommandSTAT,
    POP3Transport_CommandNOOP,
    POP3Transport_CommandRSET,
    POP3Transport_CommandUIDL,
    POP3Transport_CommandDELE,
    POP3Transport_CommandRETR
};

HRESULT WINAPI CreatePOP3Transport(IPOP3Transport **ppTransport)
{
    HRESULT hr;
    POP3Transport *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->InetTransport.u.vtblPOP3 = &POP3TransportVtbl;
    This->refs = 0;
    hr = InternetTransport_Init(&This->InetTransport);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    *ppTransport = (IPOP3Transport *)&This->InetTransport.u.vtblPOP3;
    IPOP3Transport_AddRef(*ppTransport);

    return S_OK;
}

static HRESULT WINAPI POP3TransportCF_QueryInterface(LPCLASSFACTORY iface,
    REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI POP3TransportCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI POP3TransportCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI POP3TransportCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    IPOP3Transport *pPop3Transport;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreatePOP3Transport(&pPop3Transport);
    if (FAILED(hr))
        return hr;

    hr = IPOP3Transport_QueryInterface(pPop3Transport, riid, ppv);
    IPOP3Transport_Release(pPop3Transport);

    return hr;
}

static HRESULT WINAPI POP3TransportCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d)\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl POP3TransportCFVtbl =
{
    POP3TransportCF_QueryInterface,
    POP3TransportCF_AddRef,
    POP3TransportCF_Release,
    POP3TransportCF_CreateInstance,
    POP3TransportCF_LockServer
};
static const IClassFactoryVtbl *POP3TransportCF = &POP3TransportCFVtbl;

HRESULT POP3TransportCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&POP3TransportCF, riid, ppv);
}

/*
 * SMTP Transport
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
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
 *
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

typedef struct
{
    InternetTransport InetTransport;
    ULONG refs;
    BOOL fESMTP;
    SMTPMESSAGE pending_message;
    INETADDR *addrlist;
    ULONG ulCurrentAddressIndex;
} SMTPTransport;

static HRESULT SMTPTransport_ParseResponse(SMTPTransport *This, char *pszResponse, SMTPRESPONSE *pResponse)
{
    HRESULT hrServerError;

    TRACE("response: %s\n", debugstr_a(pszResponse));

    if (!isdigit(*pszResponse))
        return IXP_E_SMTP_RESPONSE_ERROR;
    pResponse->pTransport = (ISMTPTransport *)&This->InetTransport.u.vtblSMTP2;
    pResponse->rIxpResult.pszResponse = pszResponse;
    pResponse->rIxpResult.dwSocketError = 0;
    pResponse->rIxpResult.uiServerError = strtol(pszResponse, &pszResponse, 10);
    pResponse->fDone = (*pszResponse != '-');

    switch (pResponse->rIxpResult.uiServerError)
    {
    case 211: hrServerError = IXP_E_SMTP_211_SYSTEM_STATUS; break;
    case 214: hrServerError = IXP_E_SMTP_214_HELP_MESSAGE; break;
    case 220: hrServerError = IXP_E_SMTP_220_READY; break;
    case 221: hrServerError = IXP_E_SMTP_221_CLOSING; break;
    case 245: hrServerError = IXP_E_SMTP_245_AUTH_SUCCESS; break;
    case 250: hrServerError = IXP_E_SMTP_250_MAIL_ACTION_OKAY; break;
    case 251: hrServerError = IXP_E_SMTP_251_FORWARDING_MAIL; break;
    case 334: hrServerError = IXP_E_SMTP_334_AUTH_READY_RESPONSE; break;
    case 354: hrServerError = IXP_E_SMTP_354_START_MAIL_INPUT; break;
    case 421: hrServerError = IXP_E_SMTP_421_NOT_AVAILABLE; break;
    case 450: hrServerError = IXP_E_SMTP_450_MAILBOX_BUSY; break;
    case 451: hrServerError = IXP_E_SMTP_451_ERROR_PROCESSING; break;
    case 452: hrServerError = IXP_E_SMTP_452_NO_SYSTEM_STORAGE; break;
    case 454: hrServerError = IXP_E_SMTP_454_STARTTLS_FAILED; break;
    case 500: hrServerError = IXP_E_SMTP_500_SYNTAX_ERROR; break;
    case 501: hrServerError = IXP_E_SMTP_501_PARAM_SYNTAX; break;
    case 502: hrServerError = IXP_E_SMTP_502_COMMAND_NOTIMPL; break;
    case 503: hrServerError = IXP_E_SMTP_503_COMMAND_SEQ; break;
    case 504: hrServerError = IXP_E_SMTP_504_COMMAND_PARAM_NOTIMPL; break;
    case 530: hrServerError = IXP_E_SMTP_530_STARTTLS_REQUIRED; break;
    case 550: hrServerError = IXP_E_SMTP_550_MAILBOX_NOT_FOUND; break;
    case 551: hrServerError = IXP_E_SMTP_551_USER_NOT_LOCAL; break;
    case 552: hrServerError = IXP_E_SMTP_552_STORAGE_OVERFLOW; break;
    case 553: hrServerError = IXP_E_SMTP_553_MAILBOX_NAME_SYNTAX; break;
    case 554: hrServerError = IXP_E_SMTP_554_TRANSACT_FAILED; break;
    default:
        hrServerError = IXP_E_SMTP_RESPONSE_ERROR;
        break;
    }
    pResponse->rIxpResult.hrResult = hrServerError;
    pResponse->rIxpResult.hrServerError = hrServerError;

    if (This->InetTransport.pCallback && This->InetTransport.fCommandLogging)
    {
        ITransportCallback_OnCommand(This->InetTransport.pCallback, CMD_RESP,
            pResponse->rIxpResult.pszResponse, hrServerError,
            (IInternetTransport *)&This->InetTransport.u.vtbl);
    }
    return S_OK;
}

static void SMTPTransport_CallbackDoNothing(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    TRACE("\n");
}

static void SMTPTransport_CallbackReadResponseDoNothing(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackDoNothing);
}

static void SMTPTransport_CallbackProcessDATAResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response = { 0 };
    HRESULT hr;

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    response.command = SMTP_DATA;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }
}

static void SMTPTransport_CallbackReadDATAResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackProcessDATAResponse);
}

static void SMTPTransport_CallbackProcessMAILResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response = { 0 };
    HRESULT hr;

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    response.command = SMTP_MAIL;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }
}

static void SMTPTransport_CallbackReadMAILResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackProcessMAILResponse);
}

static void SMTPTransport_CallbackProcessRCPTResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response = { 0 };
    HRESULT hr;

    TRACE("\n");

    HeapFree(GetProcessHeap(), 0, This->addrlist);
    This->addrlist = NULL;

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    response.command = SMTP_RCPT;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }
}

static void SMTPTransport_CallbackReadRCPTResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackProcessRCPTResponse);
}

static void SMTPTransport_CallbackProcessHelloResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response = { 0 };
    HRESULT hr;

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    response.command = This->fESMTP ? SMTP_EHLO : SMTP_HELO;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }

    if (!response.fDone)
    {
        InternetTransport_ReadLine(&This->InetTransport,
            SMTPTransport_CallbackProcessHelloResp);
        return;
    }

    /* FIXME: try to authorize */

    /* always changed to this status, even if authorization not support on server */
    InternetTransport_ChangeStatus(&This->InetTransport, IXP_AUTHORIZED);
    InternetTransport_ChangeStatus(&This->InetTransport, IXP_CONNECTED);

    memset(&response, 0, sizeof(response));
    response.command = SMTP_CONNECTED;
    response.fDone = TRUE;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);
}

static void SMTPTransport_CallbackRecvHelloResp(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackProcessHelloResp);
}

static void SMTPTransport_CallbackSendHello(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response = { 0 };
    HRESULT hr;
    const char *pszHello;
    char *pszCommand;
    static const char szHostName[] = "localhost"; /* FIXME */

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    response.command = SMTP_BANNER;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }

    TRACE("(%s)\n", pBuffer);

    This->fESMTP = strstr(response.rIxpResult.pszResponse, "ESMTP") &&
        This->InetTransport.ServerInfo.dwFlags & (ISF_SSLONSAMEPORT|ISF_QUERYDSNSUPPORT|ISF_QUERYAUTHSUPPORT);

    if (This->fESMTP)
        pszHello = "EHLO ";
    else
        pszHello = "HELO ";

    pszCommand = HeapAlloc(GetProcessHeap(), 0, strlen(pszHello) + strlen(szHostName) + 2);
    strcpy(pszCommand, pszHello);
    strcat(pszCommand, szHostName);
    pszCommand[strlen(pszCommand)+1] = '\0';
    pszCommand[strlen(pszCommand)] = '\n';

    InternetTransport_DoCommand(&This->InetTransport, pszCommand,
        SMTPTransport_CallbackRecvHelloResp);

    HeapFree(GetProcessHeap(), 0, pszCommand);
}

static void SMTPTransport_CallbackDisconnect(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response;
    HRESULT hr;

    TRACE("\n");

    if (pBuffer)
    {
        hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
        if (FAILED(hr))
        {
            /* FIXME: handle error */
            return;
        }

        if (FAILED(response.rIxpResult.hrServerError))
        {
            ERR("server error: %s\n", debugstr_a(pBuffer));
            /* FIXME: handle error */
            return;
        }
    }
    InternetTransport_DropConnection(&This->InetTransport);
}

static void SMTPTransport_CallbackMessageProcessResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response = { 0 };
    HRESULT hr;

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }

    response.command = SMTP_SEND_MESSAGE;
    response.rIxpResult.hrResult = S_OK;
    ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);
}

static void SMTPTransport_CallbackMessageReadResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackMessageProcessResponse);
}

static void SMTPTransport_CallbackMessageSendDOT(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    IStream_Release(This->pending_message.pstmMsg);
    InternetTransport_DoCommand(&This->InetTransport, "\n.\n",
        SMTPTransport_CallbackMessageReadResponse);
}

static void SMTPTransport_CallbackMessageSendDataStream(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response;
    HRESULT hr;
    char *pszBuffer;
    ULONG cbSize;

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }

    pszBuffer = HeapAlloc(GetProcessHeap(), 0, This->pending_message.cbSize);
    hr = IStream_Read(This->pending_message.pstmMsg, pszBuffer, This->pending_message.cbSize, NULL);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }
    cbSize = This->pending_message.cbSize;

    /* FIXME: map "\n.\n" to "\n..\n", reallocate memory, update cbSize */

    /* FIXME: properly stream the message rather than writing it all at once */

    hr = InternetTransport_Write(&This->InetTransport, pszBuffer, cbSize,
        SMTPTransport_CallbackMessageSendDOT);

    HeapFree(GetProcessHeap(), 0, pszBuffer);
}

static void SMTPTransport_CallbackMessageReadDataResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackMessageSendDataStream);
}

static void SMTPTransport_CallbackMessageSendTo(IInternetTransport *iface, char *pBuffer, int cbBuffer);

static void SMTPTransport_CallbackMessageReadToResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackMessageSendTo);
}

static void SMTPTransport_CallbackMessageSendTo(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    SMTPRESPONSE response;
    HRESULT hr;

    TRACE("\n");

    hr = SMTPTransport_ParseResponse(This, pBuffer, &response);
    if (FAILED(hr))
    {
        /* FIXME: handle error */
        return;
    }

    if (FAILED(response.rIxpResult.hrServerError))
    {
        ERR("server error: %s\n", debugstr_a(pBuffer));
        /* FIXME: handle error */
        return;
    }

    for (; This->ulCurrentAddressIndex < This->pending_message.rAddressList.cAddress; This->ulCurrentAddressIndex++)
    {
        TRACE("address[%ld]: %s\n", This->ulCurrentAddressIndex,
            This->pending_message.rAddressList.prgAddress[This->ulCurrentAddressIndex].szEmail);

        if ((This->pending_message.rAddressList.prgAddress[This->ulCurrentAddressIndex].addrtype & ADDR_TOFROM_MASK) == ADDR_TO)
        {
            static const char szCommandFormat[] = "RCPT TO: <%s>\n";
            char *szCommand;
            int len = sizeof(szCommandFormat) - 2 /* "%s" */ +
                strlen(This->pending_message.rAddressList.prgAddress[This->ulCurrentAddressIndex].szEmail);

            szCommand = HeapAlloc(GetProcessHeap(), 0, len);
            if (!szCommand)
                return;

            sprintf(szCommand, szCommandFormat,
                This->pending_message.rAddressList.prgAddress[This->ulCurrentAddressIndex].szEmail);

            This->ulCurrentAddressIndex++;
            hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
                SMTPTransport_CallbackMessageReadToResponse);

            HeapFree(GetProcessHeap(), 0, szCommand);
            return;
        }
    }

    hr = InternetTransport_DoCommand(&This->InetTransport, "DATA\n",
        SMTPTransport_CallbackMessageReadDataResponse);
}

static void SMTPTransport_CallbackMessageReadFromResponse(IInternetTransport *iface, char *pBuffer, int cbBuffer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("\n");
    InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackMessageSendTo);
}

static HRESULT WINAPI SMTPTransport_QueryInterface(ISMTPTransport2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IInternetTransport) ||
        IsEqualIID(riid, &IID_ISMTPTransport) ||
        IsEqualIID(riid, &IID_ISMTPTransport2))
    {
        *ppv = iface;
        ISMTPTransport2_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    FIXME("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI SMTPTransport_AddRef(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    return InterlockedIncrement((LONG *)&This->refs);
}

static ULONG WINAPI SMTPTransport_Release(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    ULONG refs = InterlockedDecrement((LONG *)&This->refs);
    if (!refs)
    {
        TRACE("destroying %p\n", This);
        if (This->InetTransport.Status != IXP_DISCONNECTED)
            InternetTransport_DropConnection(&This->InetTransport);

        if (This->InetTransport.pCallback) ITransportCallback_Release(This->InetTransport.pCallback);
        HeapFree(GetProcessHeap(), 0, This->addrlist);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI SMTPTransport_GetServerInfo(ISMTPTransport2 *iface,
    LPINETSERVER pInetServer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("(%p)\n", pInetServer);
    return InternetTransport_GetServerInfo(&This->InetTransport, pInetServer);
}

static IXPTYPE WINAPI SMTPTransport_GetIXPType(ISMTPTransport2 *iface)
{
    TRACE("()\n");
    return IXP_SMTP;
}

static HRESULT WINAPI SMTPTransport_IsState(ISMTPTransport2 *iface,
    IXPISSTATE isstate)
{
    FIXME("(%d): stub\n", isstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_InetServerFromAccount(
    ISMTPTransport2 *iface, IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("(%p, %p)\n", pAccount, pInetServer);
    return InternetTransport_InetServerFromAccount(&This->InetTransport, pAccount, pInetServer);
}

static HRESULT WINAPI SMTPTransport_Connect(ISMTPTransport2 *iface,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    HRESULT hr;

    TRACE("(%p, %s, %s)\n", pInetServer, fAuthenticate ? "TRUE" : "FALSE", fCommandLogging ? "TRUE" : "FALSE");

    hr = InternetTransport_Connect(&This->InetTransport, pInetServer, fAuthenticate, fCommandLogging);
    if (FAILED(hr))
        return hr;

    /* this starts the state machine, which continues in SMTPTransport_CallbackSendHELO */
    return InternetTransport_ReadLine(&This->InetTransport, SMTPTransport_CallbackSendHello);
}

static HRESULT WINAPI SMTPTransport_HandsOffCallback(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_HandsOffCallback(&This->InetTransport);
}

static HRESULT WINAPI SMTPTransport_Disconnect(ISMTPTransport2 *iface)
{
    TRACE("()\n");
    return ISMTPTransport2_CommandQUIT(iface);
}

static HRESULT WINAPI SMTPTransport_DropConnection(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_DropConnection(&This->InetTransport);
}

static HRESULT WINAPI SMTPTransport_GetStatus(ISMTPTransport2 *iface,
    IXPSTATUS *pCurrentStatus)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_GetStatus(&This->InetTransport, pCurrentStatus);
}

static HRESULT WINAPI SMTPTransport_InitNew(ISMTPTransport2 *iface,
    LPSTR pszLogFilePath, ISMTPCallback *pCallback)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("(%s, %p)\n", debugstr_a(pszLogFilePath), pCallback);

    if (!pCallback)
        return E_INVALIDARG;

    if (pszLogFilePath)
        FIXME("not using log file of %s, use Wine debug logging instead\n",
            debugstr_a(pszLogFilePath));

    ISMTPCallback_AddRef(pCallback);
    This->InetTransport.pCallback = (ITransportCallback *)pCallback;
    This->InetTransport.fInitialised = TRUE;

    return S_OK;
}

static HRESULT WINAPI SMTPTransport_SendMessage(ISMTPTransport2 *iface,
    LPSMTPMESSAGE pMessage)
{
    static const char szCommandFormat[] = "MAIL FROM: <%s>\n";
    SMTPTransport *This = (SMTPTransport *)iface;
    ULONG i, size;
    LPSTR pszFromAddress = NULL;
    char *szCommand;
    int len;
    HRESULT hr;

    TRACE("(%p)\n", pMessage);

    This->pending_message = *pMessage;
    IStream_AddRef(pMessage->pstmMsg);

    size = pMessage->rAddressList.cAddress * sizeof(INETADDR);
    This->addrlist = HeapAlloc(GetProcessHeap(), 0, size);
    if (!This->addrlist)
        return E_OUTOFMEMORY;

    memcpy(This->addrlist, pMessage->rAddressList.prgAddress, size);
    This->pending_message.rAddressList.prgAddress = This->addrlist;
    This->ulCurrentAddressIndex = 0;

    for (i = 0; i < pMessage->rAddressList.cAddress; i++)
    {
        if ((pMessage->rAddressList.prgAddress[i].addrtype & ADDR_TOFROM_MASK) == ADDR_FROM)
        {
            TRACE("address[%ld]: ADDR_FROM, %s\n", i,
                pMessage->rAddressList.prgAddress[i].szEmail);
            pszFromAddress = pMessage->rAddressList.prgAddress[i].szEmail;
        }
        else if ((pMessage->rAddressList.prgAddress[i].addrtype & ADDR_TOFROM_MASK) == ADDR_TO)
        {
            TRACE("address[%ld]: ADDR_TO, %s\n", i,
                pMessage->rAddressList.prgAddress[i].szEmail);
        }
    }

    if (!pszFromAddress)
    {
        SMTPRESPONSE response;
        memset(&response, 0, sizeof(response));
        response.command = SMTP_SEND_MESSAGE;
        response.fDone = TRUE;
        response.pTransport = (ISMTPTransport *)&This->InetTransport.u.vtblSMTP2;
        response.rIxpResult.hrResult = IXP_E_SMTP_NO_SENDER;
        ISMTPCallback_OnResponse((ISMTPCallback *)This->InetTransport.pCallback, &response);
        return S_OK;
    }
    len = sizeof(szCommandFormat) - 2 /* "%s" */ + strlen(pszFromAddress);

    szCommand = HeapAlloc(GetProcessHeap(), 0, len);
    if (!szCommand)
        return E_OUTOFMEMORY;

    sprintf(szCommand, szCommandFormat, pszFromAddress);

    hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
        SMTPTransport_CallbackMessageReadFromResponse);

    return hr;
}

static HRESULT WINAPI SMTPTransport_CommandMAIL(ISMTPTransport2 *iface, LPSTR pszEmailFrom)
{
    static const char szCommandFormat[] = "MAIL FROM: <%s>\n";
    SMTPTransport *This = (SMTPTransport *)iface;
    char *szCommand;
    int len;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_a(pszEmailFrom));

    if (!pszEmailFrom)
        return E_INVALIDARG;

    len = sizeof(szCommandFormat) - 2 /* "%s" */ + strlen(pszEmailFrom);
    szCommand = HeapAlloc(GetProcessHeap(), 0, len);
    if (!szCommand)
        return E_OUTOFMEMORY;

    sprintf(szCommand, szCommandFormat, pszEmailFrom);

    hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
        SMTPTransport_CallbackReadMAILResponse);

    HeapFree(GetProcessHeap(), 0, szCommand);
    return hr;
}

static HRESULT WINAPI SMTPTransport_CommandRCPT(ISMTPTransport2 *iface, LPSTR pszEmailTo)
{
    static const char szCommandFormat[] = "RCPT TO: <%s>\n";
    SMTPTransport *This = (SMTPTransport *)iface;
    char *szCommand;
    int len;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_a(pszEmailTo));

    if (!pszEmailTo)
        return E_INVALIDARG;

    len = sizeof(szCommandFormat) - 2 /* "%s" */ + strlen(pszEmailTo);
    szCommand = HeapAlloc(GetProcessHeap(), 0, len);
    if (!szCommand)
        return E_OUTOFMEMORY;

    sprintf(szCommand, szCommandFormat, pszEmailTo);

    hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
        SMTPTransport_CallbackReadRCPTResponse);

    HeapFree(GetProcessHeap(), 0, szCommand);
    return hr;
}

static HRESULT WINAPI SMTPTransport_CommandEHLO(ISMTPTransport2 *iface)
{
    static const char szCommandFormat[] = "EHLO %s\n";
    static const char szHostname[] = "localhost"; /* FIXME */
    SMTPTransport *This = (SMTPTransport *)iface;
    char *szCommand;
    int len = sizeof(szCommandFormat) - 2 /* "%s" */ + sizeof(szHostname);
    HRESULT hr;

    TRACE("\n");

    szCommand = HeapAlloc(GetProcessHeap(), 0, len);
    if (!szCommand)
        return E_OUTOFMEMORY;

    sprintf(szCommand, szCommandFormat, szHostname);

    hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
        SMTPTransport_CallbackReadResponseDoNothing);

    HeapFree(GetProcessHeap(), 0, szCommand);
    return hr;
}

static HRESULT WINAPI SMTPTransport_CommandHELO(ISMTPTransport2 *iface)
{
    static const char szCommandFormat[] = "HELO %s\n";
    static const char szHostname[] = "localhost"; /* FIXME */
    SMTPTransport *This = (SMTPTransport *)iface;
    char *szCommand;
    int len = sizeof(szCommandFormat) - 2 /* "%s" */ + sizeof(szHostname);
    HRESULT hr;

    TRACE("()\n");

    szCommand = HeapAlloc(GetProcessHeap(), 0, len);
    if (!szCommand)
        return E_OUTOFMEMORY;

    sprintf(szCommand, szCommandFormat, szHostname);

    hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
        SMTPTransport_CallbackReadResponseDoNothing);

    HeapFree(GetProcessHeap(), 0, szCommand);
    return hr;
}

static HRESULT WINAPI SMTPTransport_CommandAUTH(ISMTPTransport2 *iface,
    LPSTR pszAuthType)
{
    static const char szCommandFormat[] = "AUTH %s\n";
    SMTPTransport *This = (SMTPTransport *)iface;
    char *szCommand;
    int len;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_a(pszAuthType));

    if (!pszAuthType)
        return E_INVALIDARG;

    len = sizeof(szCommandFormat) - 2 /* "%s" */ + strlen(pszAuthType);
    szCommand = HeapAlloc(GetProcessHeap(), 0, len);
    if (!szCommand)
        return E_OUTOFMEMORY;

    sprintf(szCommand, szCommandFormat, pszAuthType);

    hr = InternetTransport_DoCommand(&This->InetTransport, szCommand,
        SMTPTransport_CallbackReadResponseDoNothing);

    HeapFree(GetProcessHeap(), 0, szCommand);
    return hr;
}

static HRESULT WINAPI SMTPTransport_CommandQUIT(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");

    InternetTransport_ChangeStatus(&This->InetTransport, IXP_DISCONNECTING);
    return InternetTransport_DoCommand(&This->InetTransport, "QUIT\n",
        SMTPTransport_CallbackDisconnect);
}

static HRESULT WINAPI SMTPTransport_CommandRSET(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");

    return InternetTransport_DoCommand(&This->InetTransport, "RSET\n",
        SMTPTransport_CallbackReadResponseDoNothing);
}

static HRESULT WINAPI SMTPTransport_CommandDATA(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");

    return InternetTransport_DoCommand(&This->InetTransport, "DATA\n",
        SMTPTransport_CallbackReadDATAResponse);
}

static HRESULT WINAPI SMTPTransport_CommandDOT(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_SendDataStream(ISMTPTransport2 *iface,
    IStream *pStream, ULONG cbSize)
{
    FIXME("(%p, %ld)\n", pStream, cbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_SetWindow(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_ResetWindow(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_SendMessage2(ISMTPTransport2 *iface, LPSMTPMESSAGE2 pMessage)
{
    FIXME("(%p)\n", pMessage);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandRCPT2(ISMTPTransport2 *iface, LPSTR pszEmailTo,
    INETADDRTYPE atDSN)
{
    FIXME("(%s, %u)\n", pszEmailTo, atDSN);
    return E_NOTIMPL;
}

static const ISMTPTransport2Vtbl SMTPTransport2Vtbl =
{
    SMTPTransport_QueryInterface,
    SMTPTransport_AddRef,
    SMTPTransport_Release,
    SMTPTransport_GetServerInfo,
    SMTPTransport_GetIXPType,
    SMTPTransport_IsState,
    SMTPTransport_InetServerFromAccount,
    SMTPTransport_Connect,
    SMTPTransport_HandsOffCallback,
    SMTPTransport_Disconnect,
    SMTPTransport_DropConnection,
    SMTPTransport_GetStatus,
    SMTPTransport_InitNew,
    SMTPTransport_SendMessage,
    SMTPTransport_CommandMAIL,
    SMTPTransport_CommandRCPT,
    SMTPTransport_CommandEHLO,
    SMTPTransport_CommandHELO,
    SMTPTransport_CommandAUTH,
    SMTPTransport_CommandQUIT,
    SMTPTransport_CommandRSET,
    SMTPTransport_CommandDATA,
    SMTPTransport_CommandDOT,
    SMTPTransport_SendDataStream,
    SMTPTransport_SetWindow,
    SMTPTransport_ResetWindow,
    SMTPTransport_SendMessage2,
    SMTPTransport_CommandRCPT2
};

HRESULT WINAPI CreateSMTPTransport(ISMTPTransport **ppTransport)
{
    HRESULT hr;
    SMTPTransport *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->InetTransport.u.vtblSMTP2 = &SMTPTransport2Vtbl;
    This->refs = 0;
    This->fESMTP = FALSE;
    hr = InternetTransport_Init(&This->InetTransport);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    *ppTransport = (ISMTPTransport *)&This->InetTransport.u.vtblSMTP2;
    ISMTPTransport_AddRef(*ppTransport);

    return S_OK;
}


static HRESULT WINAPI SMTPTransportCF_QueryInterface(LPCLASSFACTORY iface,
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

static ULONG WINAPI SMTPTransportCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI SMTPTransportCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI SMTPTransportCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    ISMTPTransport *pSmtpTransport;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateSMTPTransport(&pSmtpTransport);
    if (FAILED(hr))
        return hr;

    hr = ISMTPTransport_QueryInterface(pSmtpTransport, riid, ppv);
    ISMTPTransport_Release(pSmtpTransport);

    return hr;
}

static HRESULT WINAPI SMTPTransportCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d)\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl SMTPTransportCFVtbl =
{
    SMTPTransportCF_QueryInterface,
    SMTPTransportCF_AddRef,
    SMTPTransportCF_Release,
    SMTPTransportCF_CreateInstance,
    SMTPTransportCF_LockServer
};
static const IClassFactoryVtbl *SMTPTransportCF = &SMTPTransportCFVtbl;

HRESULT SMTPTransportCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&SMTPTransportCF, riid, ppv);
}

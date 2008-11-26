/*
 * Internet Messaging APIs
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

#include "winsock2.h"
#include "winuser.h"
#include "objbase.h"
#include "imnxport.h"

typedef struct InternetTransport InternetTransport;

typedef void (*INETXPORT_COMPLETION_FUNCTION)(IInternetTransport *, char *, int);

struct InternetTransport
{
    union
    {
        const IInternetTransportVtbl *vtbl;
        const ISMTPTransport2Vtbl *vtblSMTP2;
        const IIMAPTransportVtbl *vtblIMAP;
        const IPOP3TransportVtbl *vtblPOP3;
    } u;

    ITransportCallback *pCallback;
    IXPSTATUS Status;
    INETSERVER ServerInfo;
    SOCKET Socket;
    boolean fCommandLogging;
    boolean fInitialised;
    INETXPORT_COMPLETION_FUNCTION fnCompletion;
    char *pBuffer;
    int cbBuffer;
    int iCurrentBufferOffset;
    HWND hwnd;
};

HRESULT InternetTransport_Init(InternetTransport *This);
HRESULT InternetTransport_GetServerInfo(InternetTransport *This, LPINETSERVER pInetServer);
HRESULT InternetTransport_InetServerFromAccount(InternetTransport *This,
    IImnAccount *pAccount, LPINETSERVER pInetServer);
HRESULT InternetTransport_Connect(InternetTransport *This,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging);
HRESULT InternetTransport_HandsOffCallback(InternetTransport *This);
HRESULT InternetTransport_DropConnection(InternetTransport *This);
HRESULT InternetTransport_GetStatus(InternetTransport *This,
    IXPSTATUS *pCurrentStatus);
HRESULT InternetTransport_ChangeStatus(InternetTransport *This, IXPSTATUS Status);
HRESULT InternetTransport_Read(InternetTransport *This, int cbBuffer,
    INETXPORT_COMPLETION_FUNCTION fnCompletion);
HRESULT InternetTransport_ReadLine(InternetTransport *This,
    INETXPORT_COMPLETION_FUNCTION fnCompletion);
HRESULT InternetTransport_Write(InternetTransport *This, const char *pvData,
    int cbSize, INETXPORT_COMPLETION_FUNCTION fnCompletion);
HRESULT InternetTransport_DoCommand(InternetTransport *This,
    LPCSTR pszCommand, INETXPORT_COMPLETION_FUNCTION fnCompletion);

BOOL InternetTransport_RegisterClass(HINSTANCE hInstance);
void InternetTransport_UnregisterClass(HINSTANCE hInstance);

HRESULT MimeBody_create(IUnknown *outer, void **obj);
HRESULT MimeAllocator_create(IUnknown *outer, void **obj);
HRESULT MimeMessage_create(IUnknown *outer, void **obj);
HRESULT MimeSecurity_create(IUnknown *outer, void **obj);
HRESULT VirtualStream_create(IUnknown *outer, void **obj);

HRESULT MimeInternational_Construct(IMimeInternational **internat);

HRESULT SMTPTransportCF_Create(REFIID riid, LPVOID *ppv);
HRESULT IMAPTransportCF_Create(REFIID riid, LPVOID *ppv);
HRESULT POP3TransportCF_Create(REFIID riid, LPVOID *ppv);
